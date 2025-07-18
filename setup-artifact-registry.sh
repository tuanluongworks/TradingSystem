#!/bin/bash

# Setup Artifact Registry for Trading System
# This script creates the necessary Artifact Registry repository

set -e

# Configuration
PROJECT_ID=${PROJECT_ID:-"tuanluongworks"}
REGION="us-central1"
REPOSITORY_NAME="trading-system"

echo "🔧 Setting up Artifact Registry for Trading System"
echo "=============================================="
echo "Project ID: $PROJECT_ID"
echo "Region: $REGION"
echo "Repository: $REPOSITORY_NAME"
echo ""

# Check if gcloud is authenticated
echo "📋 Checking authentication..."
if ! gcloud auth list --filter=status:ACTIVE --format="value(account)" | head -n 1 > /dev/null; then
    echo "❌ No active gcloud authentication found"
    echo "Please run: gcloud auth login"
    exit 1
fi

# Set the project
echo "🎯 Setting project..."
gcloud config set project $PROJECT_ID

# Enable required APIs
echo "🔌 Enabling required APIs..."
gcloud services enable artifactregistry.googleapis.com
gcloud services enable cloudbuild.googleapis.com
gcloud services enable run.googleapis.com

# Create Artifact Registry repository
echo "📦 Creating Artifact Registry repository..."
if gcloud artifacts repositories describe $REPOSITORY_NAME --location=$REGION 2>/dev/null; then
    echo "✅ Repository $REPOSITORY_NAME already exists"
else
    gcloud artifacts repositories create $REPOSITORY_NAME \
        --repository-format=docker \
        --location=$REGION \
        --description="Docker repository for Trading System"
    echo "✅ Repository $REPOSITORY_NAME created successfully"
fi

# Configure Docker authentication
echo "🔑 Configuring Docker authentication for Artifact Registry..."
gcloud auth configure-docker $REGION-docker.pkg.dev

echo ""
echo "✅ Artifact Registry setup complete!"
echo ""
echo "📝 Next steps:"
echo "1. Build and push your image: gcloud builds submit"
echo "2. Deploy to Cloud Run using the updated cloudbuild.yaml"
echo ""
echo "🔗 Repository URL: $REGION-docker.pkg.dev/$PROJECT_ID/$REPOSITORY_NAME"
