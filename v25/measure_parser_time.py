import subprocess
import matplotlib.pyplot as plt
import time

def measure_parser_time(binary_path, test_file1, test_file2, iterations=100):
    times = []
    
    for _ in range(iterations):
        start = time.time()  # Başlangıç zamanı
        subprocess.run([binary_path, test_file1, test_file2], capture_output=True, text=True)
        end = time.time()  # Bitiş zamanı
        elapsed = end - start
        times.append(elapsed)
    
    return times

def plot_execution_times(times, title="Parser Execution Times"):
    plt.figure(figsize=(10, 6))
    plt.plot(times, marker='o', linestyle='-', label='Execution Time')
    plt.axhline(y=sum(times) / len(times), color='r', linestyle='--', label='Average Time')
    plt.title(title)
    plt.xlabel('Program Execution Number')
    plt.ylabel('Time (seconds)')
    plt.legend()
    plt.grid(True)
    plt.show()

def main():
    # Derlenmiş parser binary'sinin yolu
    parser_binary = "./parser"
    
    # Test dosyaları
    test_file1 = "test1A.c"
    test_file2 = "test2A.c"
    
    # Çalışma zamanını ölç
    times = measure_parser_time(parser_binary, test_file1, test_file2, iterations=100)
    
    # Grafik oluştur
    plot_execution_times(times)

if __name__ == "__main__":
    main()

