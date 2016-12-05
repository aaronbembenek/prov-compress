#!/usr/bin/python3
import matplotlib.pyplot as plt
from collections import defaultdict

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
                    data[curfile]["time"] = float(time)
                else:
                    size_dict = {}
                    sizes = [float(s) for s in line.split(' ')]
                    size_dict["md"] = sizes[0]
                    size_dict["id"] = sizes[1]
                    size_dict["graph"] = sizes[2]
                    size_dict["compressed"] = sizes[3]
                    size_dict["xz"] = sizes[4]
                    size_dict["original"] = sizes[5]
                    size_dict["cratio"] = sizes[6]
                    size_dict["xzratio"] = sizes[7]
                    size_dict["commonstr"] = sizes[8]
                    data[curfile]["sizes"] = size_dict
        with open(COMPRESSION_PERF_FILE, 'r') as f:
            curquery = 0
            for line in f.readlines():
                if "File" in line:
                    curfile = line
                    data[curfile]["cqueries"] = {}
                elif "Query" in line:
                    curquery = int(line.split(' ')[1])
                    data[curfile]["cqueries"][(curquery)] = {}
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
                    curquery = int(line.split(' ')[1])
                    data[curfile]["dqueries"][curquery] = {}
                elif "VM" in line:
                    vm = int(line.split(':')[1].strip())
                    data[curfile]["dqueries"][curquery]["vm"] = vm
                else:
                    data[curfile]["dqueries"][curquery]["times"] = [int(i.strip(' ')) for i in line.strip('\n').split(',')[:-1]]
        self.data = data 

    def construct_graph_data(self):
        self.queries = [0,1,2,3,4,5]
        self.x_labels = sorted(self.data.keys(), key=lambda v: int(self.data[v]["sizes"]["original"]))
        self.xz = []
        self.metadata = []
        self.graph = []
        self.sizes = []
        self.times = []
        self.dummy_qs = defaultdict(dict)
        self.compressed_qs = defaultdict(dict)
        self.dummy_vm = defaultdict(list)
        self.compressed_vm = defaultdict(list)

        for f in self.x_labels:
            self.xz.append(1.0/float(self.data[f]["sizes"]["xzratio"]))
            self.metadata.append(
                    (float(self.data[f]["sizes"]["md"]) 
                        + float(self.data[f]["sizes"]["id"])
                        + float(self.data[f]["sizes"]["commonstr"]))
                    /float(self.data[f]["sizes"]["original"]))
            self.graph.append(
                    float(self.data[f]["sizes"]["graph"])
                    /float(self.data[f]["sizes"]["original"]))
            self.sizes.append(float(self.data[f]["sizes"]["original"]))
            self.times.append(float(self.data[f]["time"]))
            for q in self.queries:
                self.dummy_qs[f].setdefault(q, []).append(list(map(float,(self.data[f]['dqueries'][q]["times"][:-1]))))
                self.compressed_qs[f].setdefault(q, []).append(list(map(float,self.data[f]['cqueries'][q]["times"][:-1])))
                self.dummy_vm[q].append(float(self.data[f]['dqueries'][q]['vm']))
                self.compressed_vm[q].append(float(self.data[f]['cqueries'][q]['vm']))

    def proportions_graph(self):
        ''' 
        bar graph plot comparing xz and us
        '''
        width = 0.35
        x1 = range(len(self.x_labels)) 
        x2 = [x+width for x in x1]
       
        fig = plt.figure()
        ax = fig.add_subplot(111)
        rects1 = ax.bar(x1, self.xz, width, color='black')
        rects2 = ax.bar(x2, self.metadata, width, color='red')
        rects3 = ax.bar(x2, self.graph, width, color='green', bottom=self.metadata)

        ax.set_xlim(-width,len(x1)+width)
        ax.set_ylim(0,0.2)
        ax.set_xlabel('Provenence Data Files (ordered by increasing size)')
        ax.set_ylabel('Proportional Size of Compressed Data')
        ax.set_title('Proportional Size of Compressed Provenance Graph (Proportionate to Original)')
        xTickMarks = self.x_labels 
        ax.set_xticks(x1)
        #xtickNames = ax.set_xticklabels(xTickMarks)
        #plt.setp(xtickNames, rotation=45, fontsize=10)
        
        ax.legend( (rects1[0], rects2[0], rects3[0]), ('XZ -9', 'Compressed Metadata', 'Compressed Graph') )
        plt.savefig("results/sizes.png")
        plt.show()
        plt.close()

    def compression_times_graph(self):
        ''' 
        plot of initial size vs. compression time 
        '''
        fig = plt.figure()
        ax = fig.add_subplot(111)

        ax.set_xlim(min(self.sizes)-100,max(self.sizes)+100)
        ax.set_xlabel('Size of Provenance Data')
        ax.set_ylabel('Time to Compress')
        ax.set_title('Time to Compress vs. Provenance Data Size')
        ax.plot(self.sizes, self.times)
        plt.savefig("results/times.png")
        plt.show()
        plt.close()

    def query_perf_graphs(self):
        width = 0.35
        x1 = range(len(self.x_labels)) 
        x2 = [x+width for x in x1]

        for q in self.queries:
            fig, axes = plt.subplots(ncols=len(self.x_labels), sharey=True)
            fig.subplots_adjust(wspace=0)
            for ax, name in zip(axes, self.x_labels):    
                ax.boxplot([self.dummy_qs[name][q], self.compressed_qs[name][q]], showfliers=False)
                ax.set(xticklabels=['Dummy', 'Compressed'])
                ax.margins(0.05) # Optional
       
            fig.text(0.5, 0.04, 'Provenance Data Files (ordered by increasing size)', ha='center')
            fig.text(0.04, 0.5, 'Time to perform 100 Queries', va='center', rotation='vertical')
            fig.suptitle('Performance of Query %d' % q)
            plt.savefig("results/perf_%d.png" % q)
            plt.show()
            plt.close()

    def query_mem_graphs(self):
        width = 0.35
        x1 = range(len(self.x_labels)) 
        x2 = [x+width for x in x1]

        # plot of memory usage
        fig = plt.figure()
        ax = fig.add_subplot(111)
        rects1 = ax.bar(x1,self.dummy_vm[0], width, color="black")
        rects2 = ax.bar(x2,self.compressed_vm[0], width, color="red")
        ax.legend( (rects1[0], rects2[0]), ('Uncompressed', 'Compressed'))
        ax.set_xlabel('Provenance Data Files (ordered by increasing size)')
        ax.set_ylabel('Virtual Memory Used (Kb)')
        ax.set_title('Virtual Memory Consumption')
        plt.savefig("results/mem.png")
        plt.show()
        plt.close()

def main():
    p = Plotter()
    p.construct_graph_data()
    #p.proportions_graph()
    #p.compression_times_graph()
    p.query_perf_graphs()
    #p.query_mem_graphs()

if __name__ == "__main__":
    main()
