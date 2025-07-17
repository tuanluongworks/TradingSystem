#!/bin/bash

# Cloud Build Trigger Setup Script
# Creates a GitHub-based Cloud Build trigger for continuous deployment

set -e

# Configuration
PROJECT_ID="tuanluongworks"
SERVICE_NAME="trading-system"
REPO_NAME="TradingSystem"
REPO_OWNER="tuanluongwork"
BRANCH_PATTERN="main"

echo "🔧 Setting up Cloud Build trigger for GitHub repository..."

# Step 1: Check if already authenticated and APIs are enabled
echo "📋 Checking prerequisites..."

# Verify project
CURRENT_PROJECT=$(gcloud config get-value project 2>/dev/null)
if [ "$CURRENT_PROJECT" != "$PROJECT_ID" ]; then
    echo "⚠️ Setting project to $PROJECT_ID"
    gcloud config set project $PROJECT_ID
fi

# Step 2: Connect GitHub repository (if not already connected)
echo "🔗 Connecting GitHub repository..."

# List existing repositories
echo "Checking if repository is already connected..."
REPO_EXISTS=$(gcloud builds repositories list --filter="name:projects/$PROJECT_ID/locations/global/connections/*/repositories/$REPO_NAME" --format="value(name)" | wc -l)

if [ "$REPO_EXISTS" -eq 0 ]; then
    echo "📝 Repository not connected. You'll need to connect it manually:"
    echo "1. Go to: https://console.cloud.google.com/cloud-build/repositories"
    echo "2. Click 'Connect Repository'"
    echo "3. Select GitHub and authorize"
    echo "4. Select repository: $REPO_OWNER/$REPO_NAME"
    echo ""
    echo "After connecting the repository, run this script again."
    read -p "Press Enter after connecting the repository..."
fi

# Step 3: Create the Cloud Build trigger
echo "🚀 Creating Cloud Build trigger..."

# Check if trigger already exists
TRIGGER_EXISTS=$(gcloud builds triggers list --filter="name:$SERVICE_NAME-trigger" --format="value(name)" | wc -l)

if [ "$TRIGGER_EXISTS" -gt 0 ]; then
    echo "⚠️ Trigger already exists. Updating..."
    gcloud builds triggers delete $SERVICE_NAME-trigger --quiet
fi

# Create the trigger
gcloud builds triggers create github \
    --repo-name="$REPO_NAME" \
    --repo-owner="$REPO_OWNER" \
    --branch-pattern="$BRANCH_PATTERN" \
    --build-config="cloudbuild.yaml" \
    --name="$SERVICE_NAME-trigger" \
    --description="Automated deployment for Trading System"

echo "✅ Cloud Build trigger created successfully!"

# Step 4: Test the trigger (optional)
echo ""
read -p "Would you like to run a test build now? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "🧪 Running test build..."
    gcloud builds triggers run $SERVICE_NAME-trigger --branch=$BRANCH_PATTERN
    echo "Build started. Check status at:"
    echo "https://console.cloud.google.com/cloud-build/builds?project=$PROJECT_ID"
fi

echo ""
echo "🎉 Setup completed!"
echo ""
echo "📋 Summary:"
echo "• Trigger name: $SERVICE_NAME-trigger"
echo "• Repository: $REPO_OWNER/$REPO_NAME"
echo "• Branch: $BRANCH_PATTERN"
echo "• Build config: cloudbuild.yaml"
echo ""
echo "🔗 Useful links:"
echo "• Cloud Build Console: https://console.cloud.google.com/cloud-build/triggers?project=$PROJECT_ID"
echo "• Cloud Run Console: https://console.cloud.google.com/run?project=$PROJECT_ID"
echo ""
echo "💡 Next time you push to the '$BRANCH_PATTERN' branch, a build will automatically start!"
