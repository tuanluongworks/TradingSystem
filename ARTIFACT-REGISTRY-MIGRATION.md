# Artifact Registry Migration Guide

## Overview
Google Container Registry (GCR) is being deprecated and shut down. This guide covers the **automatic migration** to Google Artifact Registry for the Trading System project using Google's official migration tool.

## ⚠️ Important: Use Google's Automatic Migration Tool

**DO NOT** manually change URLs in your configuration files. Google's migration tool automatically redirects `gcr.io` URLs to Artifact Registry while keeping the same URLs working.

## What the Auto-Migration Does

1. **Creates gcr.io repositories in Artifact Registry** - Your images remain accessible at the same `gcr.io/tuanluongworks/trading-system` URLs
2. **Automatically redirects traffic** - No code changes needed
3. **Copies all existing images** - All your current images are preserved
4. **Zero downtime migration** - Service continues running during migration

## Migration Steps

### Quick Migration (One Step)

**In Google Cloud Shell or system with gcloud CLI:**
```bash
# Run the quick migration script
./migrate-to-artifact-registry-quick.sh

# Or run directly:
gcloud artifacts docker upgrade migrate --projects=tuanluongworks
```

### Safer Phased Migration (Recommended for Production)

**Phase 1: Start with 1% canary reads**
```bash
./migrate-to-artifact-registry.sh
```

**Phase 2: Increase to 10% canary reads**
```bash
./migrate-to-artifact-registry-phase2.sh
```

**Phase 3: Move to 100% canary reads**
```bash
./migrate-to-artifact-registry-phase3.sh
```

**Phase 4: Complete migration**
```bash
./migrate-to-artifact-registry-complete.sh
```

### Manual Commands

If you prefer to run commands manually:

```bash
# Enable the API
gcloud services enable artifactregistry.googleapis.com

# Complete migration in one step
gcloud artifacts docker upgrade migrate --projects=tuanluongworks

# OR phased approach:
gcloud artifacts docker upgrade migrate --projects=tuanluongworks --canary-reads=1
gcloud artifacts docker upgrade migrate --projects=tuanluongworks --canary-reads=10  
gcloud artifacts docker upgrade migrate --projects=tuanluongworks --canary-reads=100
gcloud artifacts docker upgrade migrate --projects=tuanluongworks
```

## After Migration

### ✅ What Continues to Work
- **Same URLs**: `gcr.io/tuanluongworks/trading-system` continues to work
- **Existing CI/CD**: No changes needed to CloudBuild, deploy scripts, or Dockerfiles  
- **Current deployments**: Existing Cloud Run services continue running
- **Docker commands**: `docker pull gcr.io/tuanluongworks/trading-system` works as before

### 🔍 How to Verify Migration

```bash
# Check migration status
gcloud artifacts settings describe

# List repositories (should show gcr.io repos)
gcloud artifacts repositories list

# Verify images are accessible
docker pull gcr.io/tuanluongworks/trading-system:latest
```

## Rollback (if needed)

If you need to rollback during canary phase:
```bash
gcloud artifacts docker upgrade migrate --projects=tuanluongworks --canary-reads=0
```

## Error Resolution

### Current Error
```
Container Registry is deprecated and shutting down, please use the auto migration tool
```

**Solution**: Run the migration command:
```bash
gcloud artifacts docker upgrade migrate --projects=tuanluongworks
```

### Permission Issues
Ensure you have the required role:
```bash
# Check your roles
gcloud projects get-iam-policy tuanluongworks --flatten="bindings[].members" --filter="bindings.members:user:YOUR_EMAIL"

# The migration tool will suggest required permissions if missing
```

## Benefits After Migration

1. **No Code Changes Required** - Same URLs continue working
2. **Enhanced Security** - Vulnerability scanning and signing
3. **Better Performance** - Improved push/pull speeds  
4. **Multi-format Support** - Docker, Maven, npm, Python packages
5. **Fine-grained Access Control** - Better IAM integration
6. **Cost Optimization** - Improved storage management

## Repository Structure After Migration

Your images remain accessible at the same URLs:
```
gcr.io/tuanluongworks/trading-system:latest
gcr.io/tuanluongworks/trading-system:[commit-sha]
```

But are now served from Artifact Registry gcr.io repositories with enhanced features.
