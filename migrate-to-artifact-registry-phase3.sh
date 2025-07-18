#!/bin/bash

# Phase 3 of automatic migration - 100% canary reads
PROJECT_ID="tuanluongworks"

echo "📊 Phase 3: Moving to 100% canary reads..."
gcloud artifacts docker upgrade migrate \
    --projects=$PROJECT_ID \
    --canary-reads=100

echo "✅ Phase 3 complete (100% canary reads)"
echo "🔍 All reads now go to Artifact Registry, writes still go to Container Registry"
echo "🔍 Please verify everything is working correctly before completing migration."
echo ""
echo "To complete the migration, run: ./migrate-to-artifact-registry-complete.sh"
