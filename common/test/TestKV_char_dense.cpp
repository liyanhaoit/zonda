/**
 * @author fuping.yangfp@alibaba-inc.com
 */
#include "sys/time.h"
#include "pthread.h"
#include <stdint.h>
#include <iostream>
#include <google/dense_hash_map>

using namespace std;

namespace zonda {

struct eqstr {
    bool operator()(const char *s1, const char *s2) const {
        return (s1 == s2) || (s1 && s2 && strcmp(s1, s2) == 0);
    }
};

unsigned int JSHash(const char *str) {
    unsigned int hash = 1315423911;
    while (*str) {
        hash ^= ((hash << 5) + (*str++) + (hash >> 2));
    }
    return (hash & 0x7FFFFFFF);
}

class StrHash {
public:
    size_t operator()(const char *s) const {
        return JSHash(s);
    }
};

class KV {
public:
    KV();
    ~KV();
    void put(char *key, char *value);
    void remove(char *key);
    void get(char *key, char *&value);

private:
    pthread_mutex_t m_mutex;
    google::dense_hash_map<char *, char *, StrHash, eqstr> m_data;
};

KV::KV() {
    pthread_mutex_init(&m_mutex, NULL);
    m_data.set_empty_key(NULL);
    m_data.set_deleted_key("");
}

KV::~KV() {
    pthread_mutex_destroy(&m_mutex);
}

void KV::put(char *key, char *value) {
    pthread_mutex_lock(&m_mutex);
    m_data.insert(make_pair(key, value));
    pthread_mutex_unlock(&m_mutex);
}

void KV::remove(char *key) {
    pthread_mutex_lock(&m_mutex);
    m_data.erase(key);
    pthread_mutex_unlock(&m_mutex);
}

void KV::get(char *key, char *&value) {
    pthread_mutex_lock(&m_mutex);
    google::dense_hash_map<char *, char *, StrHash, eqstr>::const_iterator iter = m_data.find(key);
    if (iter == m_data.end()) {
        value = NULL;
    } else {
        value = iter->second;
    }
    pthread_mutex_unlock(&m_mutex);
}

} //namespace zonda

int main(int argc, char *argv[]) {
    using namespace zonda;

    static const int LOOP_SIZE = 10 * 1000000; // put time: 40s/10M; get time: 20s/10M; mem: 950M/10M

    KV kv;

    time_t from = time(NULL);
    for (long i = 0; i < LOOP_SIZE; ++i) {
        char *key = new char[24];
        char *value = new char[8];
        sprintf(value, ":543210");

        sprintf(key, "ABCD>%18ld", i);
        kv.put(key, value);
        if (i / 1000000 * 1000000 == i) {
            cout << "put: " << i << endl;
        }
    }
    cout << "put time: " << (time(NULL) - from) * 10000000L / LOOP_SIZE << "s per 10M" << endl;
    cout << "mem: top..." << endl;

    from = time(NULL);
    char *key = new char[24];
    char *value = new char[8];
    for (long i = 0; i < LOOP_SIZE; ++i) {
        sprintf(key, "ABCD>%18ld", i);
        kv.get(key, value);
    }
    cout << "get time: " << (time(NULL) - from) * 10000000L / LOOP_SIZE << " per 10M" << endl;

    sleep(1000000);
    return 0;
}

