# 🔧 GitHub Actions Authentication Fix Guide

## The Problem
```
google-github-actions/auth failed with: the GitHub Action workflow must specify exactly one of "workload_identity_provider" or "credentials_json"! If you are specifying input values via GitHub secrets, ensure the secret is being injected into the environment.
```

## Root Cause Analysis
This error occurs when:
1. **Missing GitHub Secret**: The `GCP_SA_KEY` secret is not configured in GitHub
2. **Empty Secret**: The secret exists but is empty or malformed
3. **Wrong Secret Name**: The secret name doesn't match what's used in the workflow
4. **Fork Limitation**: Secrets are not passed to workflows triggered from forks

## ✅ Complete Fix Solution

### Step 1: Create Service Account and Key
```cmd
# Run this script to create the service account and get the key
setup-github-auth.bat
```

### Step 2: Add Secret to GitHub Repository
1. Go to: https://github.com/tuanluongwork/TradingSystem
2. Click **Settings** → **Secrets and variables** → **Actions**
3. Click **New repository secret**
4. Name: `GCP_SA_KEY`
5. Value: Copy the entire JSON content from the script output
6. Click **Add secret**

### Step 3: Verify the Secret
The workflow now includes verification:
```yaml
- name: Verify GitHub Secrets
  run: |
    if [ -z "${{ secrets.GCP_SA_KEY }}" ]; then
      echo "❌ GCP_SA_KEY secret is not set"
      exit 1
    fi
```

## 🔍 Troubleshooting Steps

### Check 1: Verify Secret Exists
```bash
# Check if secret is configured (from GitHub UI)
# Go to Settings → Secrets and variables → Actions
# Look for: GCP_SA_KEY
```

### Check 2: Validate Service Account
```cmd
# Verify service account exists
gcloud iam service-accounts list --filter="email:github-actions@tuanluongworks.iam.gserviceaccount.com"

# Check permissions
gcloud projects get-iam-policy tuanluongworks --flatten="bindings[].members" --filter="bindings.members:serviceAccount:github-actions@tuanluongworks.iam.gserviceaccount.com"
```

### Check 3: Test Authentication Locally
```cmd
# Test the service account key locally
gcloud auth activate-service-account --key-file=github-actions-key.json
gcloud auth list
```

## 🛠️ Alternative Solutions

### Option 1: Service Account Key (Recommended for Free Tier)
```yaml
- name: Google Auth
  uses: google-github-actions/auth@v2
  with:
    credentials_json: '${{ secrets.GCP_SA_KEY }}'
```

### Option 2: Workload Identity (More Secure)
```yaml
- name: Google Auth
  uses: google-github-actions/auth@v2
  with:
    workload_identity_provider: 'projects/123456789/locations/global/workloadIdentityPools/github-pool/providers/github-provider'
    service_account: 'github-actions@tuanluongworks.iam.gserviceaccount.com'
```

## 📋 Service Account Permissions Required

The service account needs these IAM roles:
- ✅ `roles/run.admin` - Deploy to Cloud Run
- ✅ `roles/storage.admin` - Push to Container Registry  
- ✅ `roles/iam.serviceAccountUser` - Use service accounts
- ✅ `roles/cloudbuild.builds.builder` - Execute builds
- ✅ `roles/logging.logWriter` - Write build logs

## 🔐 Security Best Practices

1. **Rotate Keys Regularly**: Update service account keys every 90 days
2. **Minimum Permissions**: Only assign required roles
3. **Delete Local Keys**: Remove key files after adding to GitHub
4. **Monitor Usage**: Check service account activity in GCP Console

## 📊 Validation Commands

```bash
# Test the full workflow manually
gcloud builds submit --config cloudbuild.yaml

# Verify Cloud Run deployment
gcloud run services list --region=us-central1

# Test the deployed service
curl https://your-service-url.run.app/health
```

## 🚨 Common Issues and Fixes

### Issue: "Permission denied" during deployment
**Fix**: Run `setup-github-auth.bat` to ensure proper IAM roles

### Issue: "Project not found"
**Fix**: Verify project ID is correct in workflow file

### Issue: "Docker push failed"
**Fix**: Ensure Container Registry API is enabled

### Issue: "Service account key is invalid"
**Fix**: Regenerate the key using the setup script

## 📝 Files Updated to Fix This Issue

- ✅ `.github/workflows/deploy.yml` - Fixed authentication and added validation
- ✅ `setup-github-auth.bat/.sh` - Service account creation script
- ✅ `.github/workflows/deploy-workload-identity.yml` - Alternative workflow
- ✅ This troubleshooting guide

---

**Quick Fix Summary:**
1. Run `setup-github-auth.bat`
2. Add the JSON key as `GCP_SA_KEY` secret in GitHub
3. Push to main branch to trigger deployment

The workflow will now provide clear error messages if the secret is missing and guide you through the fix process.
