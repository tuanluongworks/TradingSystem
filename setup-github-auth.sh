#!/bin/bash

# GitHub Actions Service Account Setup Script
# Fixes the authentication error by creating proper service account and credentials

set -e

# Configuration
PROJECT_ID="tuanluongworks"
SERVICE_ACCOUNT_NAME="github-actions"
SERVICE_ACCOUNT_EMAIL="${SERVICE_ACCOUNT_NAME}@${PROJECT_ID}.iam.gserviceaccount.com"
KEY_FILE="github-actions-key.json"

echo "🔧 Setting up GitHub Actions Service Account for GCP Authentication"
echo "=================================================================="

# Step 1: Verify prerequisites
echo "📋 Step 1: Verifying prerequisites..."

# Check if gcloud is authenticated
if ! gcloud auth list --filter=status:ACTIVE --format="value(account)" | head -n 1 > /dev/null; then
    echo "❌ Not authenticated with Google Cloud"
    echo "Please run: gcloud auth login"
    exit 1
fi

# Set project
gcloud config set project $PROJECT_ID
echo "✅ Using project: $PROJECT_ID"

# Step 2: Enable required APIs
echo ""
echo "📋 Step 2: Enabling required APIs..."
gcloud services enable iam.googleapis.com --project=$PROJECT_ID
gcloud services enable cloudresourcemanager.googleapis.com --project=$PROJECT_ID
gcloud services enable cloudbuild.googleapis.com --project=$PROJECT_ID
gcloud services enable run.googleapis.com --project=$PROJECT_ID
gcloud services enable containerregistry.googleapis.com --project=$PROJECT_ID

echo "✅ APIs enabled"

# Step 3: Create service account (if it doesn't exist)
echo ""
echo "🔐 Step 3: Creating GitHub Actions service account..."

if gcloud iam service-accounts describe $SERVICE_ACCOUNT_EMAIL --project=$PROJECT_ID > /dev/null 2>&1; then
    echo "⚠️ Service account $SERVICE_ACCOUNT_EMAIL already exists"
else
    gcloud iam service-accounts create $SERVICE_ACCOUNT_NAME \
        --display-name="GitHub Actions CI/CD" \
        --description="Service account for GitHub Actions CI/CD pipeline" \
        --project=$PROJECT_ID
    echo "✅ Service account created: $SERVICE_ACCOUNT_EMAIL"
fi

# Step 4: Assign required roles
echo ""
echo "🔐 Step 4: Assigning IAM roles..."

ROLES=(
    "roles/run.admin"
    "roles/storage.admin"
    "roles/iam.serviceAccountUser"
    "roles/cloudbuild.builds.builder"
    "roles/logging.logWriter"
    "roles/containerregistry.ServiceAgent"
)

for role in "${ROLES[@]}"; do
    echo "  Assigning role: $role"
    gcloud projects add-iam-policy-binding $PROJECT_ID \
        --member="serviceAccount:$SERVICE_ACCOUNT_EMAIL" \
        --role="$role" \
        --quiet
done

echo "✅ IAM roles assigned"

# Step 5: Create and download service account key
echo ""
echo "🔑 Step 5: Creating service account key..."

# Remove existing key file if it exists
if [ -f "$KEY_FILE" ]; then
    rm "$KEY_FILE"
    echo "Removed existing key file"
fi

gcloud iam service-accounts keys create $KEY_FILE \
    --iam-account=$SERVICE_ACCOUNT_EMAIL \
    --project=$PROJECT_ID

echo "✅ Service account key created: $KEY_FILE"

# Step 6: Display the key content for GitHub Secrets
echo ""
echo "🔐 Step 6: GitHub Secrets Configuration"
echo "======================================="

echo ""
echo "📋 Add the following secret to your GitHub repository:"
echo ""
echo "Secret Name: GCP_SA_KEY"
echo "Secret Value: (copy the entire content below)"
echo ""
echo "--- START OF SECRET VALUE ---"
cat $KEY_FILE
echo ""
echo "--- END OF SECRET VALUE ---"
echo ""

# Step 7: Instructions for adding to GitHub
echo "📝 Instructions to add the secret to GitHub:"
echo ""
echo "1. Go to your GitHub repository: https://github.com/tuanluongwork/TradingSystem"
echo "2. Click on 'Settings' tab"
echo "3. In the left sidebar, click 'Secrets and variables' → 'Actions'"
echo "4. Click 'New repository secret'"
echo "5. Name: GCP_SA_KEY"
echo "6. Value: Copy the entire JSON content from above"
echo "7. Click 'Add secret'"
echo ""

# Step 8: Verify the setup
echo "🔍 Step 8: Verifying the setup..."

echo "Service account details:"
gcloud iam service-accounts describe $SERVICE_ACCOUNT_EMAIL --project=$PROJECT_ID

echo ""
echo "Service account roles:"
gcloud projects get-iam-policy $PROJECT_ID \
    --flatten="bindings[].members" \
    --format="table(bindings.role)" \
    --filter="bindings.members:serviceAccount:$SERVICE_ACCOUNT_EMAIL"

# Step 9: Security cleanup recommendation
echo ""
echo "🔒 Step 9: Security recommendations..."
echo ""
echo "⚠️ IMPORTANT SECURITY NOTES:"
echo "1. The key file '$KEY_FILE' contains sensitive credentials"
echo "2. After adding to GitHub Secrets, DELETE the local key file:"
echo "   rm $KEY_FILE"
echo "3. Never commit this key file to version control"
echo "4. Rotate the key regularly for security"
echo ""

read -p "Press Enter after you've added the secret to GitHub, then I'll clean up the key file..."

# Optional cleanup
read -p "Delete the local key file now? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    rm $KEY_FILE
    echo "✅ Local key file deleted"
else
    echo "⚠️ Remember to delete '$KEY_FILE' manually after adding to GitHub"
fi

echo ""
echo "🎉 GitHub Actions authentication setup completed!"
echo ""
echo "📝 Summary:"
echo "• Service Account: $SERVICE_ACCOUNT_EMAIL"
echo "• GitHub Secret: GCP_SA_KEY (should be added to repository)"
echo "• Required roles: Assigned ✅"
echo ""
echo "🚀 Next steps:"
echo "1. Ensure the GCP_SA_KEY secret is added to GitHub"
echo "2. Push code to main branch to trigger deployment"
echo "3. Monitor the workflow at: https://github.com/tuanluongwork/TradingSystem/actions"
