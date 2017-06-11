#ifndef  MappedArray_INC
#define  MappedArray_INC

#include <string>
#include <atomic>
#include <stdlib.h>
#include <iostream>

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <sstream>

#include <exception>
#include <unistd.h>

struct MappedArrayException: public std::exception{
    std::string message;
    MappedArrayException(std::string _message):
            message(_message){
    }
    char const * what() const noexcept override {
        return message.data();
    }
};

template <typename T>
struct MappedArray{
    //namespace io = boost::iostreams;

    int fd;
    T* base;

    size_t size;
    MappedArray(size_t elements, std::string fname): 
            size(elements){

        if(size==0){
            throw MappedArrayException("Zero length arrays are not supported");
        }
        std::string copy = fname;
        char * tmp = (char*)copy.data();
        fd = mkstemp(tmp);
        if(fd != -1){
            int err = unlink(tmp);
            if(!err){
                perror(("unlinking temp file "+fname+" failed, continuing").c_str());
            }
            fname=copy;
        } else if(errno == EINVAL){
            fd = open(fname.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        }

        int err = ftruncate(fd, sizeof(T)*elements);
        if(err){
            int err_old=errno;
            std::stringstream ss;
            ss<<"Could not resize file "<<fname<<
                "to be "<<sizeof(T)*elements<<" bytes long. Errno="
                <<err_old<<": "<<strerror(err_old)<<std::endl;

            throw MappedArrayException(ss.str());
        }

        if(fd == -1){
            throw MappedArrayException("Could not open or create file: "+fname);
        }

        void * out = mmap(NULL, elements*sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if(out == MAP_FAILED){
            char * errstr = strerror(errno);
            std::string estring(errstr);
            throw MappedArrayException("Could not mmap file, fname: "+fname+" error: "+estring);
        }

        base = reinterpret_cast<T*>(out);
    }

    T& operator[](size_t index){
        T* ret = base+index;
        return *ret;
    }

    void close(){
        int ret = munmap(base, sizeof(T)*size);
        if(ret){
            perror("Error when unmaping mapped array: ");
        }
        ::close(fd);
        size=0;
    }

        /*  Just leak since fixing this properly is a pain
    ~MappedArray(){
        int ret = munmap(base, sizeof(T)*size);
        if(ret){
            perror("Error when unmaping mapped array: ");
        }
        close(fd);
    }
        */
};
#endif   /* ----- #ifndef MappedArray_INC  ----- */
