#include <fstream>
#include <sstream>
#include <pthread.h>
#include <string>
#include <semaphore.h>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
using namespace std;

sem_t sem;
int threadCountSeekg = 0;
int WeightsSeekg = 0;
bool finalLayer = false;

struct Neuron
{
    int thread_number;
    double inputs;
    double* output;
    double result;
};

int count_threads()
{
    int num_of_threads=0;
    string line;
    ifstream fin("weights_and_inputs.txt");
    fin.seekg(threadCountSeekg);

    //counting the number of lines
    while(getline(fin, line))
    {   
        if(line == "") break;
        num_of_threads++;
    }
    threadCountSeekg = fin.tellg();
    if(fin.eof())
        finalLayer = true;
    fin.close();
    return num_of_threads;
}

int count_weights()
{
    int num_of_weights=0;
    string line;
    ifstream fin("weights_and_inputs.txt");
    fin.seekg(WeightsSeekg);
    getline(fin, line);
    stringstream iss(line);
    while(getline(iss, line, ','))
    {
        num_of_weights++;
    }
    fin.close();
    return num_of_weights;
}

void read_weights(vector<double> &weights)
{
        string line, temp;
        ifstream fin("weights_and_inputs.txt");

        fin.seekg(WeightsSeekg);
        getline(fin, line);
        stringstream iss(line);
        while (getline(iss, temp, ','))
        {
            weights.push_back(stod(temp));
        }
        //updating the seekg value for the next thread
        WeightsSeekg = fin.tellg();
        fin.close();
}


void* thread_func(void* arg)
{
    //add the linear combination of inputs and weights
    sem_wait(&sem);
    Neuron neuron = *(Neuron*)arg;
    vector<double> weights;
    read_weights(weights);

    for(int i=0; i<weights.size(); i++)
    {
        neuron.output[i] = neuron.inputs * weights[i];
    }
    cout << "Thread " << pthread_self() << " output:" << endl;
    for(int i=0; i<weights.size(); i++)
    {
        cout << neuron.output[i] << " ";
    }
    cout << endl;
    sem_post(&sem);
    pthread_exit(NULL);
}



int main(int argc, char* argv[])
{
   int process_num = 0;
   int seekg = 0;
   int numOfInputs = 2;

   if (argc > 1) {
      process_num = atoi(argv[1]);
   }
   if (argc > 2) {
      seekg = atoi(argv[2]);
   }
   if (argc >= 3) {
      numOfInputs = atoi(argv[3]);
   }

    cout << "Process number: " << process_num << endl;

    WeightsSeekg = threadCountSeekg = seekg;
    sem_init(&sem, 0, 1);
    const int NUM_OF_THREADS = count_threads();
    const int NUM_OF_WEIGHTS = count_weights();
    const int outputSize = NUM_OF_WEIGHTS;
    Neuron *neurons = new Neuron[NUM_OF_THREADS];
    double * result= new double[NUM_OF_WEIGHTS]{0.0};

    cout << "Number of threads: " << NUM_OF_THREADS << endl;

    //enter inputs
    vector<double> inputs;
    if(process_num == 0){
        double temp = 0.0;
        cout << "Enter inputs:" << endl;
        for(int i=0; i< NUM_OF_THREADS; i++){
            cin >> temp;
            inputs.push_back(temp);
        }
    }

    else
    {
        cout << "reading" << endl;
        //read from pipe
        inputs.resize(numOfInputs);
        int fd = open("pipe", O_RDONLY);
        for (int i = 0; i < NUM_OF_THREADS; i++)
        {
            sleep(1);
            double thread_input;
            read(fd, &thread_input, sizeof(double));
            inputs[i] = thread_input;
            cout << thread_input << endl;
        }
        
        // read(fd, &inputs, numOfInputs*sizeof(double));
        close(fd);
    }

    pthread_t *threads = new pthread_t[NUM_OF_THREADS];
    for(int i=0; i<NUM_OF_THREADS; i++){
        neurons[i].thread_number = i;
        neurons[i].inputs = inputs[i];
        neurons[i].output = new double[NUM_OF_WEIGHTS];
        pthread_create(&threads[i], NULL, thread_func, (void*) (&neurons[i]));
        pthread_join(threads[i], NULL);
    }

    WeightsSeekg = threadCountSeekg;


    for (int j = 0; j < NUM_OF_WEIGHTS; j++){
        for (int i = 0; i < NUM_OF_THREADS; i++){
            result[j] += neurons[i].output[j];
        }
        // cout << result[j] << endl;
    }
    
    
    if(fork() == 0){
        if(!finalLayer)
            execlp("./p1", "./p1", to_string(++process_num).c_str(), to_string(WeightsSeekg).c_str(), to_string(outputSize).c_str(),  NULL);
    }
    if (!finalLayer)
    {
        mkfifo("pipe", 0666);
        int fd = open("pipe", O_WRONLY);
        for (int i = 0; i < NUM_OF_WEIGHTS; i++)
        {
            write(fd, &result[i], sizeof(double));
        } 
        close(fd);
    }
    double output = 0;
    mkfifo("output_pipe", 0666);
    if (finalLayer)
    {
        int fd_1 = open("output_pipe", O_WRONLY);
        double amogus = 5;
        write(fd_1, &amogus, sizeof(double));
        
    }
    // while (output == 0 && !finalLayer){
        int fd_1 = open("output_pipe", O_RDONLY);
        // cout << "Process " << process_num<< endl ;
        read(fd_1, &output, sizeof(double));
        cout << "Output" << output << " from P " << process_num << endl;
    // }
    if (process_num !=1)
    {
        int fd_1 = open("output_pipe", O_WRONLY);
        write(fd_1, &output, sizeof(double));
    }
    
    
    
    pthread_exit(NULL);

}
