import subprocess
import matplotlib.pyplot as plt

def measure_with_time_command(binary_path, test_file1, test_file2, iterations=50):
    times = []
    
    for _ in range(iterations):
        result = subprocess.run(
            f'time {binary_path} {test_file1} {test_file2}',
            stderr=subprocess.PIPE,
            shell=True,
            text=True,
            executable='/bin/bash'
        )
        
        print("STDERR Output:", result.stderr)
        
        stderr_output = result.stderr
        for line in stderr_output.split('\n'):
            if "real" in line:
                time_str = line.split()[1]
                minutes, seconds = 0, 0
                if 'm' in time_str:
                    minutes, seconds = map(float, time_str.replace('s', '').split('m'))
                else:
                    seconds = float(time_str.replace('s', ''))
                elapsed_time = minutes * 60 + seconds
                times.append(elapsed_time)
                break
    
    print("Collected Times:", times)
    return times

def plot_time_command_results(times, title="Time Command Results"):
    plt.figure(figsize=(10, 6))
    plt.bar(range(1, len(times) + 1), times, color='skyblue', label='Real Time (seconds)')
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
    times = measure_with_time_command(parser_binary, test_file1, test_file2, iterations=50)
    
    # Grafik oluştur
    plot_time_command_results(times)

if __name__ == "__main__":
    main()
