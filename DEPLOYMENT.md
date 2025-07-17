# Trading System - GCP Deployment Guide

This guide will help you deploy your C++ Trading System to Google Cloud Platform using Cloud Run.

## Prerequisites

Before deploying, ensure you have:

1. **Google Cloud Account**: Your free trial account `tuanluongworks`
2. **Google Cloud SDK**: [Install gcloud CLI](https://cloud.google.com/sdk/docs/install)
3. **Docker**: [Install Docker Desktop](https://www.docker.com/products/docker-desktop/)
4. **Git**: For version control

## 🚨 IMPORTANT: Fix GCP Permissions First

If you encountered the error: "Required roles: roles/run.admin, roles/iam.serviceAccountUser", run this first:

**Windows:**
```cmd
fix-gcp-permissions.bat
```

**Linux/macOS:**
```bash
chmod +x fix-gcp-permissions.sh
./fix-gcp-permissions.sh
```

This script will:
- Enable all required APIs
- Fix Cloud Build service account permissions
- Fix Compute Engine service account permissions
- Test the setup end-to-end

## Quick Start Deployment

### 1. Set up Google Cloud Project

```bash
# Login to Google Cloud
gcloud auth login

# Set your project
gcloud config set project tuanluongworks

# Enable billing (required for Cloud Run)
# Go to: https://console.cloud.google.com/billing
```

### 2. Local Testing (Optional)

Test the application locally using Docker:

```bash
# Build and run locally
docker-compose up --build

# Test the API
curl http://localhost:8080/health
```

### 3. Deploy to GCP

#### Option A: Using the deployment script (Recommended)

**Windows:**
```cmd
deploy-gcp.bat
```

**Linux/macOS:**
```bash
chmod +x deploy-gcp.sh
./deploy-gcp.sh
```

#### Option B: Manual deployment

```bash
# Enable APIs
gcloud services enable run.googleapis.com
gcloud services enable containerregistry.googleapis.com

# Configure Docker
gcloud auth configure-docker

# Build and push
docker build -t gcr.io/tuanluongworks/trading-system:latest .
docker push gcr.io/tuanluongworks/trading-system:latest

# Deploy to Cloud Run
gcloud run deploy trading-system \
    --image gcr.io/tuanluongworks/trading-system:latest \
    --platform managed \
    --region us-central1 \
    --allow-unauthenticated \
    --memory 512Mi \
    --cpu 1 \
    --port 8080
```

## CI/CD with GitHub Actions

### 🚨 IMPORTANT: Fix GitHub Authentication First

If you get the error: "must specify exactly one of workload_identity_provider or credentials_json", run this first:

**Windows:**
```cmd
setup-github-auth.bat
```

**Linux/macOS:**
```bash
chmod +x setup-github-auth.sh
./setup-github-auth.sh
```

This will:
- Create a service account with proper permissions
- Generate a service account key
- Show you exactly what to add as a GitHub Secret

### Set up automated deployment:

1. **Create and Configure Service Account** (use the script above)

2. **Add GitHub Secret:**
   - Go to your GitHub repository → Settings → Secrets and variables → Actions
   - Add secret `GCP_SA_KEY` with the JSON content from the setup script

3. **Push to main branch** - GitHub Actions will automatically deploy!

## API Testing

Once deployed, test your API:

```bash
# Health check
curl https://your-service-url.run.app/health

# Create an order
curl -X POST https://your-service-url.run.app/api/v1/orders \
  -H "Content-Type: application/json" \
  -d '{
    "symbol": "AAPL",
    "type": "BUY",
    "quantity": 10,
    "price": 150.00
  }'
```

## Monitoring and Logs

- **Cloud Console**: https://console.cloud.google.com/run
- **Logs**: `gcloud logs tail`
- **Metrics**: Available in Cloud Console

## Cost Optimization

Your free trial includes:
- 2 million requests per month (free tier)
- 400,000 GB-seconds per month
- 200,000 CPU-seconds per month

The configuration automatically scales to zero when not in use to minimize costs.

## Troubleshooting

### Common Issues:

1. **Workload Identity Federation error**:
   ```
   Error: failed to generate Google Cloud federated token for workloadIdentityPools/github-pool
   ```
   **Solution**: Run `cleanup-workload-identity.bat` then `setup-github-auth.bat`

2. **GitHub Actions authentication failed**:
   ```
   Error: must specify exactly one of workload_identity_provider or credentials_json
   ```
   **Solution**: Run `setup-github-auth.bat` and add the GCP_SA_KEY secret to GitHub

3. **Cloud Build trigger creation failed**: 
   ```
   Error: Required roles: roles/run.admin, roles/iam.serviceAccountUser
   ```
   **Solution**: Run `fix-gcp-permissions.bat` to fix IAM permissions

4. **Build fails**: Check Docker is running and CMake version ≥ 3.20

5. **Permission denied**: Run `gcloud auth login` and check project ID

6. **Service won't start**: Check logs with `gcloud logs tail`

### Debug Commands:

```bash
# Check service status
gcloud run services describe trading-system --region=us-central1

# View logs
gcloud logs tail --filter="resource.type=cloud_run_revision"

# Test locally
docker run -p 8080:8080 gcr.io/tuanluongworks/trading-system:latest

# Check IAM permissions
gcloud projects get-iam-policy tuanluongworks
```

## 🧪 Testing the Build

Before deploying, test the Docker build locally:

```bash
# Windows
test-docker-build.bat

# Linux/Mac
./test-docker-build.sh
```

This will:
- Build the Docker image locally
- Test container startup
- Verify health endpoints
- Check for errors
- Clean up test resources

## 🚨 Enhanced Troubleshooting

### Docker Build Issues:

1. **Shell syntax error in COPY command**: 
   ```
   ERROR: failed to process "\"No": unexpected end of statement while looking for matching double-quote
   ```
   **Solution**: Fixed - COPY commands cannot use shell operators like `||` or redirection. Use separate RUN commands for shell logic.

2. **Missing data directory error**: 
   ```
   #13 [stage-1 4/7] COPY --from=builder /app/build/TradingSystem /app/ ... /app/data: not found
   ```
   **Solution**: Fixed - Dockerfile now properly copies data directory from source in builder stage.

2. **Build context too large**:
   ```
   Error: build context exceeds maximum size
   ```
   **Solution**: Check `.dockerignore` excludes build artifacts:
   ```
   build/
   .git/
   *.log
   third_party/build/
   ```

3. **CMake build fails**:
   ```
   Error: CMake version 3.20 or higher is required
   ```
   **Solution**: Dockerfile uses Ubuntu 22.04 with CMake 3.22+

### Authentication Issues:

## Security Considerations

- Change JWT secret in production configuration
- Enable authentication for production use
- Set up proper CORS policies
- Monitor API usage and set rate limits

## Next Steps

1. Set up a custom domain
2. Configure SSL certificates
3. Add database persistence (Cloud SQL)
4. Implement monitoring and alerting
5. Set up staging environment
