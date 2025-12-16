import os
import time
import redis
import socket
from PIL import Image

r = redis.Redis(host='redis', port=6379, db=0)

UPLOAD_FOLDER = '/data/uploads'
PROCESSED_FOLDER = '/data/processed'
os.makedirs(PROCESSED_FOLDER, exist_ok=True)

worker_id = socket.gethostname()

print(f"Worker {worker_id} started waiting for tasks...")

while True:
    task = r.blpop('image_queue', timeout=0)
    
    if task:
        filename = task[1].decode('utf-8')
        print(f"[{worker_id}] Processing {filename}...")

        try:
            input_path = os.path.join(UPLOAD_FOLDER, filename)
            output_path = os.path.join(PROCESSED_FOLDER, f"bw_{filename}")

            time.sleep(5) 

            with Image.open(input_path) as img:
                bw_img = img.convert('L')
                bw_img.save(output_path)
            
            print(f"[{worker_id}] Finished {filename}")
        except Exception as e:
            print(f"[{worker_id}] Error processing {filename}: {e}")