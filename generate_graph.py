import json
import matplotlib.pyplot as plt
import fire

DIMENSION_INFO = {
    'cwnds': {
        'title': 'Congestion Window (cwnd) over Time for Different CCAs',
        'ylabel': 'Congestion Window (cwnd)'
    },
    'throughputs': {
        'title': 'Throughput over Time for Different CCAs',
        'ylabel': 'Throughput (Kbps)'
    },
    'rtts': {
        'title': 'Round-Trip Time (RTT) over Time for Different CCAs',
        'ylabel': 'RTT (ms)'
    }
}

def plot_dimension(data, dimension, sample_one_cca, output_file='plot.png'):
    plt.figure(figsize=(10, 6))
    seen_ccas = set()

    for cca_data in data:
        cca = cca_data['cca']
        if sample_one_cca and cca in seen_ccas:
            continue
        seen_ccas.add(cca)
        values = cca_data[dimension]
        
        times = [point[0] for point in values]
        dim_values = [point[1] for point in values]
        
        opacity = 1.0 if cca == 'ns3::AdaptiveTcp' else 0.5

        plt.plot(times, dim_values, label=cca, linewidth=0.5, alpha=opacity)

    plt.xlabel('Time (s)')
    plt.ylabel(DIMENSION_INFO[dimension]['ylabel'])
    plt.title(DIMENSION_INFO[dimension]['title'])
    plt.legend()
    plt.grid(True)
    plt.savefig(f'plots/{dimension}_{output_file}')
    # plt.show()
    plt.clf()  # Clear the current figure for the next plot

def generate_graph(
        json_file='ns-allinone-3.43/ns-3.43/100Mbps-2ms-50p.json',
        output_file='plot.png',
        dimension='all',
        sample_one_cca=True):
    # Load the JSON data from the file
    with open(json_file, 'r') as f:
        data = json.load(f)

    dimensions = ['cwnds', 'throughputs', 'rtts']
    if dimension == 'all':
        for dim in dimensions:
            plot_dimension(data, dim, sample_one_cca)
    else:
        plot_dimension(data, dimension, sample_one_cca, output_file)

if __name__ == '__main__':
    fire.Fire(generate_graph)