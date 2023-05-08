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
vector<double> output;

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
    //last value in the line
    num_of_weights++;


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
        //adding last value to the weights
        getline(iss, temp, '\n');
        weights.push_back(stod(temp));

        //updating the seekg value for the next thread
        WeightsSeekg = fin.tellg();
        fin.close();
}

//--------------
//PROBLEM: only one thread is able to read the weights at a time
//----------------

void* thread_func(void* arg)
{
    //add the linear combination of inputs and weights
    double input = *(double*)arg;
    vector<double> weights;
    sem_wait(&sem);
    read_weights(weights);
    sem_post(&sem);

    for(int i=0; i<weights.size(); i++)
    {
        output[i] += input * weights[i];
    }
    cout << "Thread " << pthread_self() << " output:" << endl;
    for(int i=0; i<weights.size(); i++)
    {
        cout << weights[i] << " ";
    }
    cout << endl;
    pthread_exit(NULL);
}

int main(int process_num = 0, int seekg = 0, int numOfInputs = 0, char** argv = NULL)
{
    cout << "Process number: " << process_num << endl;

    WeightsSeekg = threadCountSeekg = seekg;
    sem_init(&sem, 0, 1);
    const int NUM_OF_THREADS = count_threads();
    const int NUM_OF_WEIGHTS = count_weights();
    const int outputSize = NUM_OF_WEIGHTS * NUM_OF_THREADS;
    output.resize(outputSize);

    cout << "Number of threads: " << NUM_OF_THREADS << endl;

    //enter inputs
    vector<double> inputs;
    if(process_num == 0)
    {
        cout << "aaaaaaaaaaaaaaaa" << endl;
        double temp = 0.0;
        cout << "Enter inputs:" << endl;
        for(int i=0; i<NUM_OF_THREADS; i++)
        {
        // inputs.push_back(stod(argv[i+1]));
            cin >> temp;
            inputs.push_back(temp);
        }
    }
    else
    {
        //read from pipe
        int fd = open("pipe", O_RDONLY);
        read(fd, &inputs, numOfInputs*sizeof(double));
        close(fd);
    }

    pthread_t *threads = new pthread_t[NUM_OF_THREADS];
    for(int i=0; i<NUM_OF_THREADS; i++)
    {
        pthread_create(&threads[i], NULL, thread_func, &inputs[i]);
    }
    for(int i=0; i<NUM_OF_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    mkfifo("pipe", 0666);
    int fd = open("pipe", O_WRONLY);
    write(fd, &output, output.size()*sizeof(double));
    close(fd);

    WeightsSeekg = threadCountSeekg;
    //run the same program again
    if(!finalLayer)
    execlp("./p1", "./p1", process_num++, WeightsSeekg, outputSize,  NULL);
}