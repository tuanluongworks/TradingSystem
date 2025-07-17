#!/bin/bash

# Complete GCP Setup and Fix Script
# This script fixes the Cloud Build trigger creation and IAM permission issues
# and sets up a complete CI/CD pipeline

set -e

# Configuration
PROJECT_ID="tuanluongworks"
SERVICE_NAME="trading-system"
REGION="us-central1"
REPO_NAME="TradingSystem"
REPO_OWNER="tuanluongwork"

echo "🔧 Complete GCP Setup and Fix for Trading System"
echo "=================================================="

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

# Get project number
PROJECT_NUMBER=$(gcloud projects describe ${PROJECT_ID} --format="value(projectNumber)")
echo "✅ Project: $PROJECT_ID (Number: $PROJECT_NUMBER)"

# Step 2: Enable all required APIs
echo ""
echo "📋 Step 2: Enabling required APIs..."
APIS=(
    "cloudbuild.googleapis.com"
    "run.googleapis.com"
    "containerregistry.googleapis.com"
    "iam.googleapis.com"
    "cloudresourcemanager.googleapis.com"
    "compute.googleapis.com"
    "artifactregistry.googleapis.com"
)

for api in "${APIS[@]}"; do
    echo "Enabling $api..."
    gcloud services enable $api --project=$PROJECT_ID
done

echo "✅ All APIs enabled"

# Step 3: Fix IAM permissions
echo ""
echo "🔐 Step 3: Fixing IAM permissions..."

# Service accounts
CLOUDBUILD_SA="${PROJECT_NUMBER}@cloudbuild.gserviceaccount.com"
COMPUTE_SA="${PROJECT_NUMBER}-compute@developer.gserviceaccount.com"

echo "Cloud Build SA: $CLOUDBUILD_SA"
echo "Compute Engine SA: $COMPUTE_SA"

# Roles to assign to Cloud Build service account
CLOUDBUILD_ROLES=(
    "roles/run.admin"
    "roles/iam.serviceAccountUser"
    "roles/storage.admin"
    "roles/logging.logWriter"
    "roles/cloudbuild.builds.builder"
    "roles/containerregistry.ServiceAgent"
)

echo "Assigning roles to Cloud Build service account..."
for role in "${CLOUDBUILD_ROLES[@]}"; do
    echo "  Adding role: $role"
    gcloud projects add-iam-policy-binding $PROJECT_ID \
        --member="serviceAccount:$CLOUDBUILD_SA" \
        --role="$role" \
        --quiet
done

# Roles to assign to Compute Engine service account  
COMPUTE_ROLES=(
    "roles/run.admin"
    "roles/iam.serviceAccountUser"
)

echo "Assigning roles to Compute Engine service account..."
for role in "${COMPUTE_ROLES[@]}"; do
    echo "  Adding role: $role"
    gcloud projects add-iam-policy-binding $PROJECT_ID \
        --member="serviceAccount:$COMPUTE_SA" \
        --role="$role" \
        --quiet
done

echo "✅ IAM permissions configured"

# Step 4: Verify Docker and build
echo ""
echo "🔨 Step 4: Testing Docker build..."
if docker build -t test-build . > /dev/null 2>&1; then
    echo "✅ Docker build successful"
    docker rmi test-build > /dev/null 2>&1
else
    echo "❌ Docker build failed - please check Dockerfile"
    exit 1
fi

# Step 5: Manual Cloud Build test
echo ""
echo "☁️ Step 5: Testing Cloud Build..."
echo "Submitting build to Cloud Build..."

gcloud builds submit \
    --config cloudbuild.yaml \
    --project $PROJECT_ID \
    --region $REGION \
    --quiet

echo "✅ Cloud Build test successful"

# Step 6: Set up GitHub integration (if not already done)
echo ""
echo "🔗 Step 6: GitHub Integration Setup"
echo "To complete the setup, you need to connect your GitHub repository:"
echo ""
echo "1. Go to: https://console.cloud.google.com/cloud-build/repositories?project=$PROJECT_ID"
echo "2. Click 'Connect Repository'"
echo "3. Choose 'GitHub (Cloud Build GitHub App)'"
echo "4. Authenticate and select repository: $REPO_OWNER/$REPO_NAME"
echo ""

read -p "Have you connected the GitHub repository? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Creating Cloud Build trigger..."
    
    # Check if trigger already exists
    TRIGGER_EXISTS=$(gcloud builds triggers list --filter="name:$SERVICE_NAME-trigger" --format="value(name)" | wc -l)
    
    if [ "$TRIGGER_EXISTS" -gt 0 ]; then
        echo "Updating existing trigger..."
        gcloud builds triggers delete $SERVICE_NAME-trigger --quiet
    fi
    
    # Create trigger
    gcloud builds triggers create github \
        --repo-name="$REPO_NAME" \
        --repo-owner="$REPO_OWNER" \
        --branch-pattern="main" \
        --build-config="cloudbuild.yaml" \
        --name="$SERVICE_NAME-trigger" \
        --description="Automated deployment for Trading System" \
        --project=$PROJECT_ID
    
    echo "✅ Cloud Build trigger created"
else
    echo "⚠️ Skipping trigger creation. Run setup-cloud-build-trigger.sh after connecting GitHub."
fi

# Step 7: Final verification
echo ""
echo "🔍 Step 7: Final verification..."

# Check service account permissions
echo "Verifying Cloud Build service account permissions:"
gcloud projects get-iam-policy $PROJECT_ID \
    --flatten="bindings[].members" \
    --format="table(bindings.role)" \
    --filter="bindings.members:$CLOUDBUILD_SA" | head -10

echo ""
echo "🎉 Setup completed successfully!"
echo ""
echo "📋 Summary of what was fixed:"
echo "• ✅ All required APIs enabled"
echo "• ✅ Cloud Build service account permissions fixed"
echo "• ✅ Compute Engine service account permissions fixed"
echo "• ✅ Docker build tested successfully"
echo "• ✅ Cloud Build configuration tested"
echo ""
echo "🔗 Useful links:"
echo "• Cloud Build Console: https://console.cloud.google.com/cloud-build?project=$PROJECT_ID"
echo "• Cloud Run Console: https://console.cloud.google.com/run?project=$PROJECT_ID"
echo "• IAM Console: https://console.cloud.google.com/iam-admin/iam?project=$PROJECT_ID"
echo ""
echo "💡 Next steps:"
echo "1. Push changes to your main branch to trigger automatic deployment"
echo "2. Monitor builds at: https://console.cloud.google.com/cloud-build/builds?project=$PROJECT_ID"
