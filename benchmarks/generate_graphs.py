#!/usr/bin/python3
import matplotlib.pyplot as plt

COMPRESSION_RES_FILE = "results/compression_results.data"
COMPRESSION_PERF_FILE = "results/compression_perf.data"
DUMMY_PERF_FILE = "results/dummy_perf.data"

class Plotter():
    def __init__(self):
        data = {}
        curfile = ''
        with open(COMPRESSION_RES_FILE, 'r') as f:
            for line in f.readlines():
                if "File" in line:
                    curfile = line
                    data[line] = {}
                elif "Time" in line:
                    time = line.split(':')[1].strip()
                    data[curfile]["time"] = time
                else:
                    size_dict = {}
                    sizes = line.split(' ')
                    size_dict["md"] = sizes[0]
                    size_dict["id"] = sizes[1]
                    size_dict["graph"] = sizes[2]
                    size_dict["compressed"] = sizes[3]
                    size_dict["xz"] = sizes[4]
                    size_dict["original"] = sizes[5]
                    size_dict["cratio"] = sizes[6]
                    size_dict["xzratio"] = sizes[7]
                    data[curfile]["sizes"] = size_dict
        '''
        with open(COMPRESSION_PERF_FILE, 'r') as f:
            curquery = 0
            for line in f.readlines():
                if "File" in line:
                    curfile = line
                    data[curfile]["cqueries"] = {}
                elif "Query" in line:
                    curquery = line.split(' ')[1]
                    data[curfile]["cqueries"][curquery] = {}
                elif "VM" in line:
                    vm = line.split(':')[1].strip()
                    data[curfile]["cqueries"][curquery]["vm"] = vm 
                else:
                    data[curfile]["cqueries"][curquery]["times"] = line.split(',')
               
        with open(DUMMY_PERF_FILE, 'r') as f:
            for line in f.readlines():
                if "File" in line:
                    curfile = line
                    data[curfile]["dqueries"] = {}
                elif "Query" in line:
                    curquery = line.split(' ')[1]
                    data[curfile]["dqueries"][curquery] = {}
                elif "VM" in line:
                    vm = line.split(':')[1].strip()
                    data[curfile]["dqueries"][curquery]["vm"] = vm 
                else:
                    data[curfile]["dqueries"][curquery]["times"] = line.split(',')
        '''
        self.data = data 

    def compression_size_graphs(self):
        # bar graph comparing proportion compression
        # (using 1 as original file's compression ratio)
        x_labels = sorted(self.data.keys(), key=lambda v: int(self.data[v]["sizes"]["original"]))
        xz = []
        metadata = []
        graph = []
        sizes = []
        times = []
       
        for f in x_labels:
            xz.append(1.0/float(self.data[f]["sizes"]["xzratio"]))
            metadata.append(
                    (float(self.data[f]["sizes"]["md"]) 
                        + float(self.data[f]["sizes"]["id"]))
                    /float(self.data[f]["sizes"]["original"]))
            graph.append(
                    float(self.data[f]["sizes"]["graph"])
                    /float(self.data[f]["sizes"]["original"]))
            sizes.append(float(self.data[f]["sizes"]["original"]))
            times.append(float(self.data[f]["time"]))
        
        ''' 
        bar graph plot comparing xz and us
        '''
        fig = plt.figure()
        ax = fig.add_subplot(111)
        width = 0.35
        x1 = range(len(x_labels)) 
        x2 = [x+width for x in x1]
        rects1 = ax.bar(x1, xz, width, color='black')
        rects2 = ax.bar(x2, metadata, width, color='red')
        rects3 = ax.bar(x2, graph, width, color='green', bottom=metadata)

        ax.set_xlim(-width,len(x1)+width)
        ax.set_ylim(0,0.2)
        ax.set_xlabel('Provenence Data Files (ordered by increasing size)')
        ax.set_ylabel('Proportional Size of Compressed Data')
        ax.set_title('Proportional Size of Compressed Provenance Graph (Proportionate to Original)')
        xTickMarks = x_labels 
        ax.set_xticks(x1)
        #xtickNames = ax.set_xticklabels(xTickMarks)
        #plt.setp(xtickNames, rotation=45, fontsize=10)
        
        ax.legend( (rects1[0], rects2[0], rects3[0]), ('XZ -9', 'Compressed Metadata', 'Compressed Graph') )
        plt.show()
        plt.savefig(COMPRESSION_RES_FILE + "bars.png")
        plt.close()

        ''' 
        plot of initial size vs. compression time 
        '''
        fig = plt.figure()
        ax = fig.add_subplot(111)

        ax.set_xlim(0,max(sizes)+100)
        ax.set_ylim(0,30)
        ax.set_xlabel('Size of Provenance Data')
        ax.set_ylabel('Time to Compress')
        ax.set_title('Time to Compress vs. Provenance Data Size')
        ax.plot(sizes, times)
        plt.show()
        plt.savefig(COMPRESSION_RES_FILE + "times.png")
        plt.close()

    def query_perf_graphs():
        pass            
    # box + whisker plot for compression times for both datasets
    # one for each query
 
    # plot of initial size vs. memory usage

def main():
    p = Plotter()
    p.compression_size_graphs()

if __name__ == "__main__":
    main()
