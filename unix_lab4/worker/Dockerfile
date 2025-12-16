FROM python:3.9-slim
WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt
COPY . .
# Установим PYTHONUNBUFFERED=1, чтобы видеть логи сразу
ENV PYTHONUNBUFFERED=1
CMD ["python", "worker.py"]