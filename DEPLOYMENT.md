# Deployment Guide for ARC Solver

This guide covers how to deploy and use the ARC Solver in different environments.

## 🚀 Local Installation

### Prerequisites
- Python 3.8 or higher
- pip package manager

### Installation Steps

1. **Clone the repository**:
   ```bash
   git clone https://github.com/yourusername/arc-solver.git
   cd arc-solver
   ```

2. **Create virtual environment**:
   ```bash
   python -m venv venv
   source venv/bin/activate  # On Windows: venv\Scripts\activate
   ```

3. **Install the package**:
   ```bash
   pip install -e .
   ```

4. **Verify installation**:
   ```bash
   python -c "import arc_solver; print('Installation successful!')"
   ```

## 📦 PyPI Distribution

### Building the Package

1. **Install build tools**:
   ```bash
   pip install build twine
   ```

2. **Build the package**:
   ```bash
   python -m build
   ```

3. **Upload to PyPI** (if you have access):
   ```bash
   twine upload dist/*
   ```

### Installing from PyPI

```bash
pip install arc-solver
```

## 🐳 Docker Deployment

### Dockerfile

```dockerfile
FROM python:3.9-slim

WORKDIR /app

COPY requirements.txt .
RUN pip install -r requirements.txt

COPY . .
RUN pip install -e .

ENTRYPOINT ["python", "-m", "arc_solver.cli"]
```

### Building and Running

```bash
# Build image
docker build -t arc-solver .

# Run container
docker run -v $(pwd)/data:/app/data arc-solver input.json -o output.json
```

## ☁️ Cloud Deployment

### AWS Lambda

1. **Create deployment package**:
   ```bash
   pip install -r requirements.txt -t package/
   cp -r arc_solver package/
   cd package && zip -r ../lambda-deployment.zip .
   ```

2. **Deploy to Lambda** with handler: `arc_solver.lambda_handler.handler`

### Google Cloud Functions

1. **Create requirements.txt** for cloud deployment
2. **Deploy using gcloud CLI**:
   ```bash
   gcloud functions deploy arc-solver \
     --runtime python39 \
     --trigger-http \
     --entry-point handler
   ```

## 🔧 Configuration

### Environment Variables

```bash
export ARC_SOLVER_MAX_CANDIDATES=10
export ARC_SOLVER_TIMEOUT=300
export ARC_SOLVER_ENABLE_ICECUBE=true
```

### Configuration Files

Create `config.json`:
```json
{
  "enable_repeating_solver": true,
  "enable_grid_solver": true,
  "enable_ml_solver": true,
  "max_candidates": 5,
  "timeout_seconds": 300
}
```

## 📊 Performance Optimization

### Memory Optimization

- **Batch processing**: Process tasks in batches to manage memory
- **Garbage collection**: Explicitly call GC after processing large tasks
- **Memory profiling**: Use `memory_profiler` to identify bottlenecks

### Speed Optimization

- **Parallel processing**: Use multiprocessing for independent tasks
- **Caching**: Cache intermediate results for repeated patterns
- **Early termination**: Stop processing when good solutions are found

## 🔍 Monitoring and Logging

### Logging Configuration

```python
import logging

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
```

### Metrics Collection

```python
from arc_solver.utils.metrics import MetricsCollector

metrics = MetricsCollector()
metrics.record_solver_performance(solver_name, execution_time, success_rate)
```

## 🧪 Testing in Production

### Load Testing

```bash
# Test with multiple tasks
python -m arc_solver.cli batch_input.json --max-candidates 3

# Performance benchmark
python examples/benchmark.py
```

### Integration Testing

```bash
# Run integration tests
pytest tests/test_integration.py

# Test with real ARC data
python examples/test_with_real_data.py
```

## 🔒 Security Considerations

- **Input validation**: Validate all input files
- **Resource limits**: Set appropriate time and memory limits
- **Error handling**: Implement proper error handling and logging
- **Access control**: Restrict access to sensitive configuration

## 📈 Scaling

### Horizontal Scaling

- **Load balancer**: Distribute tasks across multiple instances
- **Queue system**: Use Redis/RabbitMQ for task queuing
- **Auto-scaling**: Configure auto-scaling based on load

### Vertical Scaling

- **Memory optimization**: Increase memory allocation
- **CPU optimization**: Use more CPU cores for parallel processing
- **Storage optimization**: Use faster storage for large datasets

## 🆘 Troubleshooting

### Common Issues

1. **Memory errors**: Reduce batch size or increase memory allocation
2. **Timeout errors**: Increase timeout or optimize solver performance
3. **Import errors**: Check Python path and package installation
4. **Performance issues**: Profile code and optimize bottlenecks

### Debug Mode

```bash
# Enable debug logging
export ARC_SOLVER_DEBUG=true
python -m arc_solver.cli input.json --verbose
```

## 📞 Support

For deployment issues:

1. Check the troubleshooting section
2. Review logs and error messages
3. Create an issue on GitHub with detailed information
4. Contact the maintainers with specific error details 