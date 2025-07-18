#!/bin/bash

# Quick one-step migration from Container Registry to Artifact Registry
# This performs the complete migration in one step (less safe but faster)

set -e

PROJECT_ID="tuanluongworks"

echo "⚡ Quick migration from Container Registry to Artifact Registry"
echo "============================================================="
echo "Project: $PROJECT_ID"
echo ""
echo "⚠️  WARNING: This performs the complete migration in one step"
echo "⚠️  For production systems, consider using the phased approach"
echo ""

# Check authentication
if ! gcloud auth list --filter=status:ACTIVE --format="value(account)" | head -n 1 > /dev/null; then
    echo "❌ No active gcloud authentication found"
    echo "Please run: gcloud auth login"
    exit 1
fi

# Set project and enable APIs
gcloud config set project $PROJECT_ID
gcloud services enable artifactregistry.googleapis.com

echo "🚀 Starting complete migration..."
gcloud artifacts docker upgrade migrate --projects=$PROJECT_ID

echo ""
echo "🎉 Migration complete!"
echo "✅ All Container Registry traffic now redirected to Artifact Registry"
echo "✅ Your existing gcr.io URLs will continue to work!"
echo ""
echo "🔗 Images are now served from Artifact Registry at:"
echo "   gcr.io/$PROJECT_ID/* (redirected automatically)"
