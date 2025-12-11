#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstdio>

using namespace std;

struct ComplexData {
    int value;
};

mutex g_mutex;
condition_variable g_cond;
ComplexData* g_message = nullptr;
bool g_ready = false;

void run_provider() {
    int id_counter = 0;

    while (true) {
        this_thread::sleep_for(chrono::seconds(1));

        ComplexData* data = new ComplexData;
        data->value = ++id_counter;

        unique_lock<mutex> lock(g_mutex);

        while (g_ready) {
            g_cond.wait(lock);
        }

        g_message = data;
        g_ready = true;

        printf("Provider: sent event %d\n", data->value);

        lock.unlock();
        g_cond.notify_one();
    }
}

void run_consumer() {
    while (true) {
        unique_lock<mutex> lock(g_mutex);

        while (!g_ready) {
            g_cond.wait(lock);
        }

        ComplexData* received = g_message;
        g_message = nullptr;
        g_ready = false;

        printf("Consumer: processed event %d\n", received->value);

        lock.unlock();
        g_cond.notify_one();

        delete received;
    }
}

int main() {
    thread t_prov(run_provider);
    thread t_cons(run_consumer);

    t_prov.join();
    t_cons.join();

    return 0;
}