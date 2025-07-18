#!/bin/bash

# Phase 2 of automatic migration - 10% canary reads
PROJECT_ID="tuanluongworks"

echo "📊 Phase 2: Increasing to 10% canary reads..."
gcloud artifacts docker upgrade migrate \
    --projects=$PROJECT_ID \
    --canary-reads=10

echo "✅ Phase 2 complete (10% canary reads)"
echo "🔍 Please verify everything is working correctly before proceeding."
echo ""
echo "To continue, run: ./migrate-to-artifact-registry-phase3.sh"
