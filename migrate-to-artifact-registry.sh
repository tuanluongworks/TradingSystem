#!/bin/bash

# Automatic migration from Container Registry to Artifact Registry
# This script uses Google's official migration tool to automatically migrate
# from gcr.io to Artifact Registry hosted gcr.io repositories

set -e

PROJECT_ID="tuanluongworks"

echo "🚀 Starting automatic migration from Container Registry to Artifact Registry"
echo "============================================================================"
echo "Project: $PROJECT_ID"
echo ""

# Check if gcloud is authenticated
echo "📋 Checking authentication..."
if ! gcloud auth list --filter=status:ACTIVE --format="value(account)" | head -n 1 > /dev/null; then
    echo "❌ No active gcloud authentication found"
    echo "Please run: gcloud auth login"
    exit 1
fi

echo "✅ Authentication verified"
echo ""

# Set the project
echo "🎯 Setting project..."
gcloud config set project $PROJECT_ID
echo ""

# Enable required APIs
echo "🔌 Enabling Artifact Registry API..."
gcloud services enable artifactregistry.googleapis.com
echo ""

# Run the automatic migration
echo "🔄 Starting automatic migration..."
echo "This will:"
echo "  1. Create gcr.io repositories in Artifact Registry"
echo "  2. Suggest and apply IAM policies"
echo "  3. Redirect traffic from gcr.io to Artifact Registry"
echo "  4. Copy all container images"
echo "  5. Disable request-time copying"
echo ""

# Start with canary reads for safer migration
echo "📊 Phase 1: Starting with 1% canary reads..."
gcloud artifacts docker upgrade migrate \
    --projects=$PROJECT_ID \
    --canary-reads=1

echo ""
echo "✅ Phase 1 complete (1% canary reads)"
echo "🔍 Please verify everything is working correctly before proceeding."
echo ""
echo "To continue the migration, run:"
echo "  ./migrate-to-artifact-registry-phase2.sh"
echo ""
echo "Or run the commands manually:"
echo "  # Phase 2: 10% canary reads"
echo "  gcloud artifacts docker upgrade migrate --projects=$PROJECT_ID --canary-reads=10"
echo ""
echo "  # Phase 3: 100% canary reads"
echo "  gcloud artifacts docker upgrade migrate --projects=$PROJECT_ID --canary-reads=100"
echo ""
echo "  # Phase 4: Complete migration"
echo "  gcloud artifacts docker upgrade migrate --projects=$PROJECT_ID"
