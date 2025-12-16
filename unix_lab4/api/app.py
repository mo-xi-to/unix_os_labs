import os
from flask import Flask, request, jsonify
import redis

app = Flask(__name__)
r = redis.Redis(host='redis', port=6379, db=0)

UPLOAD_FOLDER = '/data/uploads'
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

@app.route('/upload', methods=['POST'])
def upload_image():
    if 'file' not in request.files:
        return jsonify({"error": "No file part"}), 400
    
    file = request.files['file']
    if file.filename == '':
        return jsonify({"error": "No selected file"}), 400

    file_path = os.path.join(UPLOAD_FOLDER, file.filename)
    file.save(file_path)

    r.rpush('image_queue', file.filename)

    return jsonify({"message": f"Task for {file.filename} queued"}), 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)