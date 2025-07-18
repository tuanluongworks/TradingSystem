#!/bin/bash

# Complete the automatic migration - Final phase
PROJECT_ID="tuanluongworks"

echo "🏁 Final Phase: Completing migration..."
echo "This will redirect both reads AND writes to Artifact Registry"
gcloud artifacts docker upgrade migrate \
    --projects=$PROJECT_ID

echo ""
echo "🎉 Migration complete!"
echo ""
echo "✅ All Container Registry traffic now redirected to Artifact Registry"
echo "✅ All images copied to Artifact Registry"
echo "✅ No more dependency on Container Registry"
echo ""
echo "🔗 Your images are now available at:"
echo "   gcr.io/$PROJECT_ID/* (redirected to Artifact Registry)"
echo ""
echo "📝 No code changes required - existing gcr.io URLs will continue to work!"
