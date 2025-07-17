# 🔧 Workload Identity Federation Error Fix

## The Problem
```
google-github-actions/auth failed with: failed to generate Google Cloud federated token for //iam.googleapis.com/projects/123456789/locations/global/workloadIdentityPools/github-pool/providers/github-provider: {"error":"invalid_target","error_description":"The target service indicated by the \"audience\" parameters is invalid. This might either be because the pool or provider is disabled or deleted or because it doesn't exist."}
```

## Root Cause Analysis
This error occurs when:
1. **Workload Identity Pool doesn't exist** - The referenced pool/provider was never created
2. **Incorrect configuration** - The pool/provider IDs are wrong or malformed
3. **Wrong authentication method** - Using workload identity when service account key is simpler
4. **Conflicting workflows** - Multiple authentication methods configured

## ✅ Complete Fix Solution

### Step 1: Clean Up Conflicting Configuration
```cmd
# Remove any workload identity configuration
cleanup-workload-identity.bat

# Run diagnostics to identify issues
diagnose-github-auth.bat
```

### Step 2: Switch to Service Account Key Authentication (Recommended)
```cmd
# Set up proper service account and key
setup-github-auth.bat
```

### Step 3: Verify Workflow Configuration
Ensure your `.github/workflows/deploy.yml` contains:
```yaml
- name: Google Auth
  uses: google-github-actions/auth@v2
  with:
    credentials_json: '${{ secrets.GCP_SA_KEY }}'
    project_id: ${{ env.PROJECT_ID }}
```

And does **NOT** contain:
```yaml
# ❌ Remove this if present
workload_identity_provider: 'projects/123456789/locations/global/workloadIdentityPools/github-pool/providers/github-provider'
```

## 🔍 Diagnostic Steps

### Check 1: Verify Authentication Method
```cmd
# Check which authentication method your workflow is using
findstr /C:"workload_identity_provider" .github\workflows\deploy.yml
findstr /C:"credentials_json" .github\workflows\deploy.yml
```

### Check 2: Check for Workload Identity Pools
```cmd
# List any existing workload identity pools
gcloud iam workload-identity-pools list --location=global --project=tuanluongworks
```

### Check 3: Verify Service Account Exists
```cmd
# Check if GitHub Actions service account exists
gcloud iam service-accounts list --filter="email:github-actions@tuanluongworks.iam.gserviceaccount.com"
```

## 🛠️ Three Authentication Options

### Option 1: Service Account Key (Recommended for Free Tier)
**Pros:** Simple setup, works immediately, no additional configuration
**Cons:** Requires secure storage of JSON key

```yaml
- name: Google Auth
  uses: google-github-actions/auth@v2
  with:
    credentials_json: '${{ secrets.GCP_SA_KEY }}'
    project_id: ${{ env.PROJECT_ID }}
```

### Option 2: Workload Identity Federation (Advanced)
**Pros:** More secure, no long-lived keys
**Cons:** Complex setup, requires additional GCP configuration

```yaml
- name: Google Auth
  uses: google-github-actions/auth@v2
  with:
    workload_identity_provider: 'projects/PROJECT_NUMBER/locations/global/workloadIdentityPools/github-pool/providers/github-provider'
    service_account: 'github-actions@tuanluongworks.iam.gserviceaccount.com'
```

### Option 3: Direct Service Account Impersonation
**Pros:** No secrets to manage
**Cons:** Requires specific IAM setup

## 🔧 Quick Fix Commands

### Fix 1: Remove Workload Identity and Use Service Account Key
```cmd
# Clean up any conflicting configuration
cleanup-workload-identity.bat

# Set up service account key authentication
setup-github-auth.bat

# Add the JSON key to GitHub Secrets as GCP_SA_KEY
```

### Fix 2: Properly Configure Workload Identity (Advanced)
```cmd
# Get your project number
gcloud projects describe tuanluongworks --format="value(projectNumber)"

# Create workload identity pool
gcloud iam workload-identity-pools create github-pool \
    --location="global" \
    --description="GitHub Actions pool" \
    --display-name="GitHub Actions"

# Create workload identity provider
gcloud iam workload-identity-pools providers create-oidc github-provider \
    --workload-identity-pool="github-pool" \
    --location="global" \
    --issuer-uri="https://token.actions.githubusercontent.com" \
    --attribute-mapping="google.subject=assertion.sub,attribute.actor=assertion.actor,attribute.repository=assertion.repository"

# Allow GitHub Actions to impersonate service account
gcloud iam service-accounts add-iam-policy-binding \
    github-actions@tuanluongworks.iam.gserviceaccount.com \
    --role="roles/iam.workloadIdentityUser" \
    --member="principalSet://iam.googleapis.com/projects/PROJECT_NUMBER/locations/global/workloadIdentityPools/github-pool/attribute.repository/tuanluongwork/TradingSystem"
```

## 📋 Verification Checklist

- [ ] Only one authentication method configured in workflow
- [ ] GCP_SA_KEY secret exists in GitHub repository (if using service account key)
- [ ] Service account has required IAM roles
- [ ] Required APIs are enabled
- [ ] No conflicting workload identity pools (unless intentionally using them)
- [ ] Workflow file syntax is correct

## 🚨 Common Mistakes to Avoid

1. **Mixing authentication methods** - Don't use both workload identity and service account key
2. **Wrong project number** - Ensure project number (not ID) is used in workload identity
3. **Incorrect pool/provider names** - Names must match exactly
4. **Missing IAM bindings** - Service account needs proper roles
5. **Typos in workflow** - YAML syntax is strict

## 📝 Files Updated to Fix This Issue

- ✅ `cleanup-workload-identity.bat/.sh` - Removes conflicting workload identity config
- ✅ `diagnose-github-auth.bat/.sh` - Comprehensive diagnostics
- ✅ Updated `.github/workflows/deploy.yml` - Fixed authentication configuration
- ✅ This troubleshooting guide

---

**Quick Fix Summary:**
1. Run `cleanup-workload-identity.bat` to remove conflicts
2. Run `setup-github-auth.bat` to set up service account key
3. Add JSON key as `GCP_SA_KEY` secret in GitHub
4. Verify workflow uses `credentials_json` (not `workload_identity_provider`)
5. Push to main branch to test deployment

**For Free Tier Users:** Use service account key authentication - it's simpler and more reliable for development purposes.
