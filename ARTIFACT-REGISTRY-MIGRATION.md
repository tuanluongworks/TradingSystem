# Artifact Registry Migration Guide

## Overview
Google Container Registry (GCR) is being deprecated and shut down. This guide covers the migration to Google Artifact Registry for the Trading System project.

## What Changed

### Before (GCR)
```
gcr.io/tuanluongworks/trading-system
```

### After (Artifact Registry)
```
us-central1-docker.pkg.dev/tuanluongworks/trading-system/trading-system
```

## Migration Steps

### 1. Setup Artifact Registry Repository

**On Windows:**
```cmd
# Run the setup script
.\setup-artifact-registry.bat
```

**On Linux/macOS:**
```bash
# Run the setup script
./setup-artifact-registry.sh
```

**Or manually:**
```bash
# Enable APIs
gcloud services enable artifactregistry.googleapis.com
gcloud services enable cloudbuild.googleapis.com
gcloud services enable run.googleapis.com

# Create repository
gcloud artifacts repositories create trading-system \
    --repository-format=docker \
    --location=us-central1 \
    --description="Docker repository for Trading System"

# Configure Docker authentication
gcloud auth configure-docker us-central1-docker.pkg.dev
```

### 2. Updated Files

The following files have been updated to use Artifact Registry:

- `cloudbuild.yaml` - Updated image URLs and push targets
- `cloudrun-service.yaml` - Updated container image reference
- `deploy-gcp.sh` - Updated image names and Docker configuration
- `deploy-gcp.bat` - Updated image names and Docker configuration

### 3. Deploy the Changes

**Option A: Using Cloud Build (Recommended)**
```bash
gcloud builds submit --config cloudbuild.yaml
```

**Option B: Using local deployment script**
```bash
# On Linux/macOS
./deploy-gcp.sh

# On Windows
.\deploy-gcp.bat
```

## Error Resolution

### If you see the error:
```
Container Registry is deprecated and shutting down, please use the auto migration tool to migrate to Artifact Registry
```

**Quick Fix:**
1. Run the setup script: `.\setup-artifact-registry.bat`
2. Commit and push the updated configuration files
3. Re-run your deployment

### Auto-migration (if available)
```bash
gcloud artifacts docker upgrade migrate --projects='tuanluongworks'
```

## Verification

After migration, verify the setup:

```bash
# List repositories
gcloud artifacts repositories list --location=us-central1

# List images
gcloud artifacts docker images list us-central1-docker.pkg.dev/tuanluongworks/trading-system

# Test deployment
gcloud run services describe trading-system --region=us-central1
```

## Benefits of Artifact Registry

1. **Enhanced Security**: Vulnerability scanning and signing
2. **Better Performance**: Improved push/pull speeds
3. **Multi-format Support**: Docker, Maven, npm, Python packages
4. **Fine-grained Access Control**: IAM integration
5. **Cost Optimization**: Better storage management

## Repository Structure

```
us-central1-docker.pkg.dev/
└── tuanluongworks/                 # Project ID
    └── trading-system/             # Repository name
        └── trading-system/         # Image name
            ├── latest              # Latest tag
            └── [commit-sha]        # Commit-specific tags
```

## Troubleshooting

### Authentication Issues
```bash
gcloud auth login
gcloud auth configure-docker us-central1-docker.pkg.dev
```

### Permission Issues
```bash
# Ensure you have the required roles
gcloud projects add-iam-policy-binding tuanluongworks \
    --member="user:your-email@domain.com" \
    --role="roles/artifactregistry.writer"
```

### Repository Not Found
```bash
# Recreate the repository
gcloud artifacts repositories create trading-system \
    --repository-format=docker \
    --location=us-central1
```
