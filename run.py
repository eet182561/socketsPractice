import sys
import wave

def process_wav(input_file):
    for i in range(10):
        output_file = f"output{i+1}.wav"
        with wave.open(input_file, 'rb') as in_wav:
            with wave.open(output_file, 'wb') as out_wav:
                out_wav.setparams(in_wav.getparams())
                frames = in_wav.readframes(in_wav.getnframes())
                out_wav.writeframes(frames)
        print(f"Generated {output_file}")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python3 run.py <input_wav_file>")
        sys.exit(1)

    input_wav_file = sys.argv[1]
    process_wav(input_wav_file)
