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

"""
CCA: ns3::TcpBbr, Throughput: 2.93 Mbps
CCA: ns3::TcpBic, Throughput: 23.888 Mbps
CCA: ns3::TcpHtcp, Throughput: 16.762 Mbps
CCA: ns3::TcpIllinois, Throughput: 9.798 Mbps
CCA: ns3::TcpNewReno, Throughput: 9.443 Mbps
CCA: ns3::TcpVegas, Throughput: 0.172 Mbps
CCA: ns3::TcpVeno, Throughput: 9.386 Mbps
CCA: ns3::TcpCubic, Throughput: 17.208 Mbps
"""

"""
Reformat in form:
<Variant name>: <Throghput quantity>
TcpBbr: 2.93 Mbps
TcpBic: 23.888 Mbps
TcpHtcp: 16.762 Mbps
TcpIllinois: 9.798 Mbps
TcpNewReno: 9.443 Mbps
TcpVegas: 0.172 Mbps
TcpVeno: 9.386 Mbps
TcpCubic: 17.208 Mbps
"""

"""

allowed_ccas = ["ns3::TcpCubic", "ns3::TcpBbr", "TcpAdaptive", "ns3::TcpAdaptive", "ns3::TcpHtcp", "ns3::TcpIllinois", "ns3::TcpNewReno", "ns3::TcpVegas", "ns3::TcpVeno"]
# allowed_ccas = ["ns3::AdaptiveTcp"]


def plot_dimension(data, dimension, sample_one_cca, output_file='plot.png'):
    plt.figure(figsize=(10, 6))
    seen_ccas = set()

    for cca_data in data:
        cca = cca_data['cca']
        if (sample_one_cca and cca in seen_ccas) or (cca not in allowed_ccas):
            continue
        seen_ccas.add(cca)
        values = cca_data[dimension]
        
        combined_data = [[point[0], point[1]] for point in values]
        # point[0] is a vallue in seconds. it can be very granular. i want to round it to the nearest millisecond and remove any points with duplicate point[0]
        combined_data = list(dict.fromkeys([(round(point[0], 3), point[1]) for point in combined_data]))
        combined_data = sorted(combined_data, key=lambda x: x[0])
        combined_data2 = []
        seen_times = set()

        for point in combined_data:
            # print(point[0])
            if point[0] not in seen_times:
                combined_data2.append(point)
                seen_times.add(point[0])

        # if (dimension == "cwnds"):
        #     print(combined_data2)

        times = []
        dim_values = []
        last_time = -100
        delta = 0.03

        for time, val in combined_data2:
            if time - last_time < delta: continue

            last_time = time

            # print(time, val)

            times.append(time)
            dim_values.append(val)

        # print(len(times), len(dim_values))
        # times = [point[0] for point in combined_data]
        # dim_values = [point[1] for point in combined_data]
        
        opacity = 1.0 if cca == 'ns3::AdaptiveTcp' else 1.0

        # if dimension == 'cwnds':
            # print(times)
            # print(dim_values)

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
        json_file='ns-allinone-3.43/ns-3.43/100Mbps-18ms-7000p.json',
        output_file='plot.png',
        dimension='all',
        sample_one_cca=True):
    # Load the JSON data from the file
    data = None

    with open(json_file, 'r') as f:
        data = json.load(f)

    dimensions = ['cwnds', 'throughputs', 'rtts']
    
    # print(data[2].keys())

    if dimension == 'all':
        for dim in dimensions:
            plot_dimension(data, dim, sample_one_cca)
    else:
        plot_dimension(data, dimension, sample_one_cca, output_file)

if __name__ == '__main__':
    fire.Fire(generate_graph)