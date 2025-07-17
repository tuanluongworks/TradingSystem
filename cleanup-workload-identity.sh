#!/bin/bash

# Clean up Workload Identity Configuration Script
# Removes any existing workload identity pools that might be causing conflicts

set -e

PROJECT_ID="tuanluongworks"

echo "🔧 Cleaning up Workload Identity Configuration"
echo "=============================================="

# Step 1: Check if authenticated
if ! gcloud auth list --filter=status:ACTIVE --format="value(account)" | head -n 1 > /dev/null; then
    echo "❌ Not authenticated with gcloud"
    echo "Please run: gcloud auth login"
    exit 1
fi

gcloud config set project $PROJECT_ID

# Step 2: List existing workload identity pools
echo "📋 Checking for existing workload identity pools..."
POOLS=$(gcloud iam workload-identity-pools list --location=global --project=$PROJECT_ID --format="value(name)" 2>/dev/null || echo "")

if [ -z "$POOLS" ]; then
    echo "✅ No workload identity pools found"
else
    echo "⚠️ Found workload identity pools:"
    echo "$POOLS"
    
    read -p "Do you want to delete these pools? This will switch to service account key authentication. (y/n): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "🗑️ Deleting workload identity pools..."
        
        for pool in $POOLS; do
            # Extract pool ID from full name
            POOL_ID=$(basename "$pool")
            echo "Deleting pool: $POOL_ID"
            
            # List and delete providers first
            PROVIDERS=$(gcloud iam workload-identity-pools providers list \
                --workload-identity-pool="$POOL_ID" \
                --location=global \
                --project=$PROJECT_ID \
                --format="value(name)" 2>/dev/null || echo "")
            
            for provider in $PROVIDERS; do
                PROVIDER_ID=$(basename "$provider")
                echo "  Deleting provider: $PROVIDER_ID"
                gcloud iam workload-identity-pools providers delete "$PROVIDER_ID" \
                    --workload-identity-pool="$POOL_ID" \
                    --location=global \
                    --project=$PROJECT_ID \
                    --quiet
            done
            
            # Delete the pool
            gcloud iam workload-identity-pools delete "$POOL_ID" \
                --location=global \
                --project=$PROJECT_ID \
                --quiet
            
            echo "✅ Deleted pool: $POOL_ID"
        done
        
        echo "✅ All workload identity pools cleaned up"
    else
        echo "⚠️ Keeping existing workload identity pools"
        echo "Note: Make sure they are properly configured or remove them manually"
    fi
fi

# Step 3: Verify GitHub workflow is using correct authentication
echo ""
echo "📋 Checking GitHub workflow configuration..."

WORKFLOW_FILE=".github/workflows/deploy.yml"
if [ -f "$WORKFLOW_FILE" ]; then
    if grep -q "workload_identity_provider" "$WORKFLOW_FILE"; then
        echo "⚠️ Found workload identity configuration in $WORKFLOW_FILE"
        echo "Updating to use service account key authentication..."
        
        # Backup the file
        cp "$WORKFLOW_FILE" "${WORKFLOW_FILE}.backup"
        
        # Remove workload identity lines and ensure service account key is used
        sed -i 's/workload_identity_provider.*//g' "$WORKFLOW_FILE"
        sed -i 's/service_account.*//g' "$WORKFLOW_FILE"
        
        echo "✅ Updated workflow to use service account key authentication"
        echo "Backup saved as: ${WORKFLOW_FILE}.backup"
    else
        echo "✅ Workflow is already configured for service account key authentication"
    fi
else
    echo "⚠️ Workflow file not found: $WORKFLOW_FILE"
fi

# Step 4: Instructions for next steps
echo ""
echo "🎯 Next Steps"
echo "============="
echo ""
echo "1. Run the authentication setup:"
echo "   ./setup-github-auth.sh"
echo ""
echo "2. Add the service account key to GitHub Secrets:"
echo "   - Go to: https://github.com/tuanluongwork/TradingSystem/settings/secrets/actions"
echo "   - Add secret: GCP_SA_KEY"
echo "   - Value: The JSON content from setup-github-auth.sh"
echo ""
echo "3. Verify the workflow uses service account key authentication:"
echo "   - Check .github/workflows/deploy.yml"
echo "   - Should contain: credentials_json: '\${{ secrets.GCP_SA_KEY }}'"
echo "   - Should NOT contain: workload_identity_provider"
echo ""
echo "4. Test the deployment:"
echo "   - Push to main branch"
echo "   - Monitor GitHub Actions"
echo ""

echo "✅ Workload identity cleanup completed!"
