#!/bin/bash
# Quick deployment test script

echo "🧪 Testing Trading System Deployment..."

# Check if Docker is running
if ! docker info > /dev/null 2>&1; then
    echo "❌ Docker is not running. Please start Docker Desktop."
    exit 1
fi

# Check if gcloud is installed
if ! command -v gcloud &> /dev/null; then
    echo "❌ Google Cloud SDK is not installed."
    echo "Please install it from: https://cloud.google.com/sdk/docs/install"
    exit 1
fi

# Check if logged in to gcloud
if ! gcloud auth list --filter=status:ACTIVE --format="value(account)" | head -n 1 > /dev/null; then
    echo "❌ Not logged in to Google Cloud."
    echo "Please run: gcloud auth login"
    exit 1
fi

# Test local Docker build
echo "🔨 Testing Docker build..."
if docker build -t trading-system-test . > /dev/null 2>&1; then
    echo "✅ Docker build successful"
    docker rmi trading-system-test > /dev/null 2>&1
else
    echo "❌ Docker build failed"
    exit 1
fi

# Check project configuration
PROJECT_ID=$(gcloud config get-value project 2>/dev/null)
if [ "$PROJECT_ID" = "tuanluongworks" ]; then
    echo "✅ Project configured correctly: $PROJECT_ID"
else
    echo "⚠️ Project ID is '$PROJECT_ID', expected 'tuanluongworks'"
    echo "Run: gcloud config set project tuanluongworks"
fi

echo ""
echo "🚀 Ready for deployment!"
echo "Run ./deploy-gcp.sh to deploy to Google Cloud Run"
