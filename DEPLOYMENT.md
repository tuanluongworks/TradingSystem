# Trading System - GCP Deployment Guide

This guide will help you deploy your C++ Trading System to Google Cloud Platform using Cloud Run.

## Prerequisites

Before deploying, ensure you have:

1. **Google Cloud Account**: Your free trial account `tuanluongworks`
2. **Google Cloud SDK**: [Install gcloud CLI](https://cloud.google.com/sdk/docs/install)
3. **Docker**: [Install Docker Desktop](https://www.docker.com/products/docker-desktop/)
4. **Git**: For version control

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

### Set up automated deployment:

1. **Create a Service Account:**
   ```bash
   gcloud iam service-accounts create github-actions \
       --display-name="GitHub Actions"
   
   gcloud projects add-iam-policy-binding tuanluongworks \
       --member="serviceAccount:github-actions@tuanluongworks.iam.gserviceaccount.com" \
       --role="roles/run.admin"
   
   gcloud projects add-iam-policy-binding tuanluongworks \
       --member="serviceAccount:github-actions@tuanluongworks.iam.gserviceaccount.com" \
       --role="roles/storage.admin"
   
   gcloud iam service-accounts keys create github-actions-key.json \
       --iam-account=github-actions@tuanluongworks.iam.gserviceaccount.com
   ```

2. **Add GitHub Secrets:**
   - Go to your GitHub repository → Settings → Secrets and variables → Actions
   - Add secret `GCP_SA_KEY` with the contents of `github-actions-key.json`

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

1. **Build fails**: Check Docker is running and CMake version ≥ 3.20
2. **Permission denied**: Run `gcloud auth login` and check project ID
3. **Service won't start**: Check logs with `gcloud logs tail`

### Debug Commands:

```bash
# Check service status
gcloud run services describe trading-system --region=us-central1

# View logs
gcloud logs tail --filter="resource.type=cloud_run_revision"

# Test locally
docker run -p 8080:8080 gcr.io/tuanluongworks/trading-system:latest
```

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
