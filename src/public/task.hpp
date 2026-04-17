namespace tskr
{
    class task
    {
    private:
         int _counter = 0;
    public:
        task(int counter) : _counter(counter) {}
        ~task();
    };
    
    task::~task()
    {
    }
    
} // namespace tskr
