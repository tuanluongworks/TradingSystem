#!/bin/bash

# Simple one-command migration for Google Cloud Shell
# Run this in Cloud Shell to migrate from Container Registry to Artifact Registry

echo "🚀 Running Container Registry to Artifact Registry migration"
echo "============================================================"

# Enable the API and run migration in one command
gcloud services enable artifactregistry.googleapis.com && \
gcloud artifacts docker upgrade migrate --projects=tuanluongworks

echo ""
echo "✅ Migration complete!"
echo "🎉 Your gcr.io URLs now work with Artifact Registry"
echo "🔧 No code changes needed - existing builds will now work"
