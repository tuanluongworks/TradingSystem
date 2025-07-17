#!/bin/bash

# GitHub Actions Authentication Diagnostic Script
# Helps troubleshoot authentication issues and verify setup

set -e

# Configuration
PROJECT_ID="tuanluongworks"
SERVICE_ACCOUNT_NAME="github-actions"
SERVICE_ACCOUNT_EMAIL="${SERVICE_ACCOUNT_NAME}@${PROJECT_ID}.iam.gserviceaccount.com"

echo "🔍 GitHub Actions Authentication Diagnostics"
echo "=============================================="

# Step 1: Check if authenticated with gcloud
echo "📋 Step 1: Checking gcloud authentication..."
if gcloud auth list --filter=status:ACTIVE --format="value(account)" | head -n 1 > /dev/null; then
    ACTIVE_ACCOUNT=$(gcloud auth list --filter=status:ACTIVE --format="value(account)" | head -n 1)
    echo "✅ Authenticated as: $ACTIVE_ACCOUNT"
else
    echo "❌ Not authenticated with gcloud"
    echo "Please run: gcloud auth login"
    exit 1
fi

# Step 2: Check project configuration
echo ""
echo "📋 Step 2: Checking project configuration..."
CURRENT_PROJECT=$(gcloud config get-value project 2>/dev/null)
if [ "$CURRENT_PROJECT" = "$PROJECT_ID" ]; then
    echo "✅ Project configured: $PROJECT_ID"
else
    echo "⚠️ Project mismatch. Current: $CURRENT_PROJECT, Expected: $PROJECT_ID"
    gcloud config set project $PROJECT_ID
    echo "✅ Project updated to: $PROJECT_ID"
fi

# Step 3: Check if service account exists
echo ""
echo "📋 Step 3: Checking service account..."
if gcloud iam service-accounts describe $SERVICE_ACCOUNT_EMAIL --project=$PROJECT_ID > /dev/null 2>&1; then
    echo "✅ Service account exists: $SERVICE_ACCOUNT_EMAIL"
    
    # Check when it was created
    CREATED=$(gcloud iam service-accounts describe $SERVICE_ACCOUNT_EMAIL --project=$PROJECT_ID --format="value(oauth2ClientId)")
    echo "   Service account details verified"
else
    echo "❌ Service account does not exist: $SERVICE_ACCOUNT_EMAIL"
    echo "Please run: setup-github-auth.bat to create it"
    exit 1
fi

# Step 4: Check service account permissions
echo ""
echo "📋 Step 4: Checking service account permissions..."
echo "Required roles for GitHub Actions deployment:"

REQUIRED_ROLES=(
    "roles/run.admin"
    "roles/storage.admin"
    "roles/iam.serviceAccountUser"
    "roles/cloudbuild.builds.builder"
    "roles/logging.logWriter"
)

for role in "${REQUIRED_ROLES[@]}"; do
    if gcloud projects get-iam-policy $PROJECT_ID \
        --flatten="bindings[].members" \
        --filter="bindings.role:$role AND bindings.members:serviceAccount:$SERVICE_ACCOUNT_EMAIL" \
        --format="value(bindings.role)" | grep -q "$role"; then
        echo "✅ $role"
    else
        echo "❌ $role - MISSING"
        echo "   Run: gcloud projects add-iam-policy-binding $PROJECT_ID --member=\"serviceAccount:$SERVICE_ACCOUNT_EMAIL\" --role=\"$role\""
    fi
done

# Step 5: Check required APIs
echo ""
echo "📋 Step 5: Checking required APIs..."
REQUIRED_APIS=(
    "cloudbuild.googleapis.com"
    "run.googleapis.com"
    "containerregistry.googleapis.com"
    "iam.googleapis.com"
)

for api in "${REQUIRED_APIS[@]}"; do
    if gcloud services list --enabled --filter="name:$api" --format="value(name)" | grep -q "$api"; then
        echo "✅ $api"
    else
        echo "❌ $api - DISABLED"
        echo "   Run: gcloud services enable $api --project=$PROJECT_ID"
    fi
done

# Step 6: Check for workload identity configuration (should not exist)
echo ""
echo "📋 Step 6: Checking for conflicting workload identity configuration..."
if gcloud iam workload-identity-pools list --location=global --project=$PROJECT_ID --format="value(name)" 2>/dev/null | grep -q "github-pool"; then
    echo "⚠️ Workload Identity pool 'github-pool' found"
    echo "   This might be causing conflicts. For simplicity, we recommend using service account keys."
    echo "   If you want to use workload identity, ensure it's properly configured."
else
    echo "✅ No conflicting workload identity pools found"
fi

# Step 7: Test service account key generation
echo ""
echo "📋 Step 7: Testing service account key generation..."
TEST_KEY_FILE="test-key-$(date +%s).json"
if gcloud iam service-accounts keys create $TEST_KEY_FILE \
    --iam-account=$SERVICE_ACCOUNT_EMAIL \
    --project=$PROJECT_ID > /dev/null 2>&1; then
    echo "✅ Service account key generation works"
    
    # Test authentication with the key
    if gcloud auth activate-service-account --key-file=$TEST_KEY_FILE --quiet; then
        echo "✅ Service account key authentication works"
        
        # Test basic GCP operation
        if gcloud projects describe $PROJECT_ID --format="value(projectId)" > /dev/null 2>&1; then
            echo "✅ Service account can access project"
        else
            echo "❌ Service account cannot access project"
        fi
    else
        echo "❌ Service account key authentication failed"
    fi
    
    # Clean up test key
    rm $TEST_KEY_FILE
    gcloud iam service-accounts keys list --iam-account=$SERVICE_ACCOUNT_EMAIL --format="value(name)" | tail -1 | xargs gcloud iam service-accounts keys delete --iam-account=$SERVICE_ACCOUNT_EMAIL --quiet
    echo "✅ Test key cleaned up"
else
    echo "❌ Cannot generate service account key"
fi

# Step 8: Summary and recommendations
echo ""
echo "🎯 Summary and Recommendations"
echo "=============================="

echo ""
echo "If all checks above passed:"
echo "1. Run 'setup-github-auth.bat' to create a fresh service account key"
echo "2. Add the JSON key as 'GCP_SA_KEY' secret in GitHub repository"
echo "3. Ensure you're using the main deploy.yml workflow (not workload identity)"
echo ""

echo "If you see failures above:"
echo "1. Fix any missing roles or APIs"
echo "2. Re-run this diagnostic script"
echo "3. Then proceed with the setup"
echo ""

echo "GitHub Repository Secrets:"
echo "- Go to: https://github.com/tuanluongwork/TradingSystem/settings/secrets/actions"
echo "- Ensure 'GCP_SA_KEY' secret exists and contains valid JSON"
echo ""

echo "Workflow files:"
echo "- Should use: .github/workflows/deploy.yml"
echo "- Should NOT use workload identity unless properly configured"
echo ""

echo "🔧 Quick fix commands:"
echo "# Enable APIs"
echo "gcloud services enable cloudbuild.googleapis.com run.googleapis.com containerregistry.googleapis.com iam.googleapis.com --project=$PROJECT_ID"
echo ""
echo "# Fix permissions"
echo "./setup-github-auth.sh"
echo ""

echo "✅ Diagnostic completed!"
