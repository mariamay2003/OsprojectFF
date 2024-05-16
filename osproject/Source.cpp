#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <algorithm>
#include <tuple>
using namespace std;
class PCB {
public:
    string pid;
    int arrival_time;
    int burst_time;
    int remaining_time;
    int start_time;
    int finish_time;
    int waiting_time;
    int turnaround_time;

    PCB(string id, int arrival, int burst)
        : pid(id), arrival_time(arrival), burst_time(burst),
        remaining_time(burst), start_time(0), finish_time(0),
        waiting_time(0), turnaround_time(0) {}

    void reset() {
        remaining_time = burst_time;
        start_time = 0;
        finish_time = 0;
        waiting_time = 0;
        turnaround_time = 0;
    }
};

void read_processes(const std::string& filename, std::vector<PCB>& processes, int& quantum, int& context_switch) {
    ifstream file(filename);
    string line;
    if (file.is_open()) {
        getline(file, line);
        quantum = stoi(line);
        getline(file, line);
        context_switch = stoi(line);
        while (getline(file, line)) {
            size_t pos1 = line.find(',');
            size_t pos2 = line.find(',', pos1 + 1);
            string pid = line.substr(0, pos1);
            int arrival_time = stoi(line.substr(pos1 + 1, pos2 - pos1 - 1));
            int burst_time = std::stoi(line.substr(pos2 + 1));
            processes.push_back(PCB(pid, arrival_time, burst_time));
        }
    }
    else {
        cerr << "Failed to open file: " << filename << std::endl;
    }
}

void simulate_fcfs(vector<PCB>& processes, int context_switch, vector<std::tuple<string, int, int>>& gantt_chart, double& cpu_utilization) {
    int current_time = 0;
    int total_burst_time = 0;
    for (PCB& process : processes) {
        if (current_time < process.arrival_time) {
            current_time = process.arrival_time;
        }
        if (!gantt_chart.empty()) {
            current_time += context_switch;
        }
        process.start_time = current_time;
        gantt_chart.push_back(make_tuple(process.pid, current_time, current_time + process.burst_time));
        current_time += process.burst_time;
        process.finish_time = current_time;
        process.waiting_time = process.start_time - process.arrival_time;
        process.turnaround_time = process.finish_time - process.arrival_time;
        total_burst_time += process.burst_time;
    }
    cpu_utilization = (double)total_burst_time / current_time * 100;
}

void simulate_srt(vector<PCB>& processes, int context_switch, vector<std::tuple<string, int, int>>& gantt_chart, double& cpu_utilization) {
    int current_time = 0;
    int total_burst_time = 0;
    vector<PCB*> ready_queue;
    auto comp = [](PCB* a, PCB* b) { return a->remaining_time > b->remaining_time; };

    // Initialize processes in the queue
    vector<PCB*> all_processes(processes.size());
    transform(processes.begin(), processes.end(), all_processes.begin(), [](PCB& p) { return &p; });

    while (!all_processes.empty() || !ready_queue.empty()) {
        // Add processes to the ready queue based on the current time
        auto it = remove_if(all_processes.begin(), all_processes.end(), [&](PCB* p) {
            if (p->arrival_time <= current_time) {
                ready_queue.push_back(p);
                return true;
            }
            return false;
            });
        all_processes.erase(it, all_processes.end());
        sort(ready_queue.begin(), ready_queue.end(), comp);

        if (ready_queue.empty()) {
            if (!all_processes.empty()) {
                current_time = all_processes.front()->arrival_time; // Jump to next process arrival if there is a gap
            }
            continue;
        }

        PCB* process = ready_queue.front();
        if (!gantt_chart.empty() && get<0>(gantt_chart.back()) != process->pid) {
            current_time += context_switch;
        }

        // Execute process for one unit of time
        gantt_chart.push_back(make_tuple(process->pid, current_time, current_time + 1));
        current_time += 1;
        process->remaining_time -= 1;

        // Check if process is finished
        if (process->remaining_time == 0) {
            process->finish_time = current_time;
            process->turnaround_time = process->finish_time - process->arrival_time;
            process->waiting_time = process->turnaround_time - process->burst_time;
            total_burst_time += process->burst_time;
            ready_queue.erase(ready_queue.begin());
        }
    }
    cpu_utilization = (double)total_burst_time / current_time * 100;
}


void simulate_rr(vector<PCB>& processes, int quantum, int context_switch, vector<tuple<std::string, int, int>>& gantt_chart, double& cpu_utilization) {
    int current_time = 0;
    int total_burst_time = 0;
    queue<PCB*> queue;

    // Load all processes into a temporary all processes list
    vector<PCB*> all_processes(processes.size());
    transform(processes.begin(), processes.end(), all_processes.begin(), [](PCB& p) { return &p; });

    while (!all_processes.empty() || !queue.empty()) {
        // Enqueue processes that have arrived by the current time
        for (auto it = all_processes.begin(); it != all_processes.end();) {
            if ((*it)->arrival_time <= current_time) {
                queue.push(*it);
                it = all_processes.erase(it);
            }
            else {
                ++it;
            }
        }

        if (queue.empty()) {
            if (!all_processes.empty()) {
                current_time = all_processes.front()->arrival_time;
            }
            continue;
        }

        PCB* process = queue.front();
        queue.pop();

        if (!gantt_chart.empty() && get<0>(gantt_chart.back()) != process->pid) {
            current_time += context_switch; // Apply context switch time only if switching processes
        }

        int execution_time = min(process->remaining_time, quantum);
        gantt_chart.push_back(make_tuple(process->pid, current_time, current_time + execution_time));
        current_time += execution_time;
        process->remaining_time -= execution_time;

        // Check if the process has remaining time and re-enqueue if needed
        if (process->remaining_time > 0) {
            queue.push(process);
        }
        else {
            process->finish_time = current_time;
            process->turnaround_time = process->finish_time - process->arrival_time;
            process->waiting_time = process->turnaround_time - process->burst_time;
            total_burst_time += process->burst_time;
        }
    }
    cpu_utilization = (double)total_burst_time / current_time * 100;
}


void print_gantt_chart(const vector<tuple<string, int, int>>& gantt_chart) {
    cout << "Gantt Chart:\n";
    for (const auto& item : gantt_chart) {
        string pid;
        int start, end;
        tie(pid, start, end) = item;
        cout << pid << "[" << start << "-" << end << "] ";
    }
    cout << endl;
}

void print_results(const std::vector<PCB>& processes, double cpu_utilization) {
    for (const PCB& process : processes) {
        std::cout << "Process " << process.pid << ", Finish: " << process.finish_time
            << ", Waiting: " << process.waiting_time << ", Turnaround: " << process.turnaround_time << endl;
    }
    cout << "CPU Utilization: " << cpu_utilization << "%" << endl;
}

int main() {
    string filename ;
    cout << "Enter the filename of the process data: ";
    cin >> filename;

    vector<PCB> processes;
    int quantum, context_switch;
    read_processes(filename, processes, quantum, context_switch);

    cout << "\nFCFS:\n";
    vector<PCB> fcfs_processes = processes;
    vector<std::tuple<std::string, int, int>> fcfs_gantt_chart;
    double fcfs_cpu_utilization;
    simulate_fcfs(fcfs_processes, context_switch, fcfs_gantt_chart, fcfs_cpu_utilization);
    print_gantt_chart(fcfs_gantt_chart);
    print_results(fcfs_processes, fcfs_cpu_utilization);

    cout << "\nSRT:\n";
    vector<PCB> srt_processes = processes;
    vector<tuple<string, int, int>> srt_gantt_chart;
    double srt_cpu_utilization;
    simulate_srt(srt_processes, context_switch, srt_gantt_chart, srt_cpu_utilization);
    print_gantt_chart(srt_gantt_chart);
    print_results(srt_processes, srt_cpu_utilization);

    cout << "\nRR:\n";
    vector<PCB> rr_processes = processes;
    vector<std::tuple<std::string, int, int>> rr_gantt_chart;
    double rr_cpu_utilization;
    simulate_rr(rr_processes, quantum, context_switch, rr_gantt_chart, rr_cpu_utilization);
    print_gantt_chart(rr_gantt_chart);
    print_results(rr_processes, rr_cpu_utilization);

    return 0;
}