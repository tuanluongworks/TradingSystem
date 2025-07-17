#!/bin/bash

# GCP IAM and Cloud Build Setup Script
# Fixes the Cloud Build trigger creation and IAM permissions issues

set -e

# Configuration
PROJECT_ID="tuanluongworks"
SERVICE_NAME="trading-system"
REGION="us-central1"

echo "🔧 Setting up Google Cloud Build and IAM permissions..."

# Step 1: Get project number (needed for service accounts)
PROJECT_NUMBER=$(gcloud projects describe ${PROJECT_ID} --format="value(projectNumber)")
echo "📋 Project Number: ${PROJECT_NUMBER}"

# Step 2: Enable required APIs
echo "📋 Enabling required APIs..."
gcloud services enable cloudbuild.googleapis.com --project=${PROJECT_ID}
gcloud services enable run.googleapis.com --project=${PROJECT_ID}
gcloud services enable containerregistry.googleapis.com --project=${PROJECT_ID}
gcloud services enable iam.googleapis.com --project=${PROJECT_ID}
gcloud services enable cloudresourcemanager.googleapis.com --project=${PROJECT_ID}

# Step 3: Set up Cloud Build service account permissions
echo "🔐 Configuring Cloud Build service account permissions..."

# Default Cloud Build service account
CLOUDBUILD_SA="${PROJECT_NUMBER}@cloudbuild.gserviceaccount.com"

# Add Cloud Run Admin role to Cloud Build service account
gcloud projects add-iam-policy-binding ${PROJECT_ID} \
    --member="serviceAccount:${CLOUDBUILD_SA}" \
    --role="roles/run.admin"

# Add Service Account User role to Cloud Build service account
gcloud projects add-iam-policy-binding ${PROJECT_ID} \
    --member="serviceAccount:${CLOUDBUILD_SA}" \
    --role="roles/iam.serviceAccountUser"

# Add Storage Admin role for Container Registry
gcloud projects add-iam-policy-binding ${PROJECT_ID} \
    --member="serviceAccount:${CLOUDBUILD_SA}" \
    --role="roles/storage.admin"

# Add Logs Writer role
gcloud projects add-iam-policy-binding ${PROJECT_ID} \
    --member="serviceAccount:${CLOUDBUILD_SA}" \
    --role="roles/logging.logWriter"

# Step 4: Set up Compute Engine default service account permissions
echo "🔐 Configuring Compute Engine default service account..."

# Default Compute Engine service account
COMPUTE_SA="${PROJECT_NUMBER}-compute@developer.gserviceaccount.com"

# Add Cloud Run Admin role to Compute Engine service account
gcloud projects add-iam-policy-binding ${PROJECT_ID} \
    --member="serviceAccount:${COMPUTE_SA}" \
    --role="roles/run.admin"

# Add Service Account User role to Compute Engine service account
gcloud projects add-iam-policy-binding ${PROJECT_ID} \
    --member="serviceAccount:${COMPUTE_SA}" \
    --role="roles/iam.serviceAccountUser"

echo "✅ IAM permissions configured successfully!"

# Step 5: Verify permissions
echo "🔍 Verifying permissions..."

echo "Cloud Build service account roles:"
gcloud projects get-iam-policy ${PROJECT_ID} \
    --flatten="bindings[].members" \
    --format="table(bindings.role)" \
    --filter="bindings.members:${CLOUDBUILD_SA}"

echo ""
echo "Compute Engine default service account roles:"
gcloud projects get-iam-policy ${PROJECT_ID} \
    --flatten="bindings[].members" \
    --format="table(bindings.role)" \
    --filter="bindings.members:${COMPUTE_SA}"

echo ""
echo "✅ Setup completed successfully!"
echo "You can now create Cloud Build triggers or run manual deployments."
echo ""
echo "📝 Next steps:"
echo "1. Create a Cloud Build trigger:"
echo "   gcloud builds triggers create github --repo-name=TradingSystem --repo-owner=tuanluongwork --branch-pattern=main --build-config=cloudbuild.yaml"
echo ""
echo "2. Or run a manual build:"
echo "   gcloud builds submit --config cloudbuild.yaml ."
