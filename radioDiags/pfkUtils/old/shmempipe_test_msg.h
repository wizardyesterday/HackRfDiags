
#define SHMEMFILE "SHMEMPIPE"
#define FIFOMASTERSLAVE "FIFOMASTERSLAVE"
#define FIFOSLAVEMASTER "FIFOSLAVEMASTER"

struct MyTestMsg : public shmempipeMessage
{
    static MyTestMsg * allocSize(shmempipe * pPipe) {
        return (MyTestMsg *) pPipe->allocSize(sizeof(MyTestMsg), true);
    }
    static const int max_size = 184;
    int seqno;
    char data[max_size];
};
