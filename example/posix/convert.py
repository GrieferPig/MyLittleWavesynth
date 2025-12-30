import wave
import struct
import os

SAMPLE_RATE = 44100
CHANNELS = 1
SAMPLE_WIDTH = 2 # 16 bit

def main():
    if not os.path.exists("output.raw"):
        print("output.raw not found. Run the C program first.")
        return

    try:
        with open("output.raw", "rb") as raw_file:
            raw_data = raw_file.read()
            
        with wave.open("output.wav", "w") as wav_file:
            wav_file.setnchannels(CHANNELS)
            wav_file.setsampwidth(SAMPLE_WIDTH)
            wav_file.setframerate(SAMPLE_RATE)
            wav_file.writeframes(raw_data)
            
        print("Converted output.raw to output.wav")
    except Exception as e:
        print(f"Error converting file: {e}")

if __name__ == "__main__":
    main()
