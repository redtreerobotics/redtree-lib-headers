#ifndef RTR_H
#define RTR_H

#include "rt_types.h"

#include <stdint.h>		//uint32_t
#include <thread>		//std::thread
#include <vector>		//std::vector
#include <string>		//std::string
#include <map>			//std::map
#include <functional>	//std::function
#include <iostream>		//std::cout

/*
 * All of the main "system" related classes and structures are located in here
 * m_lock, m_lockable, m_option_interface, m_subscription_interface, m_tag, m_external, m_option,
 * m_subscribable, m_value_subscription, m_stoplight_subscription, m_module, m_device, m_fpga
 * 
 * Note: the types of tags available are in their own header
 * Note: the following user libraries are in their own header:
 * - cloud dB
 * - seamless networking
 * 
 * Todo: move the following code into their own header
 * - user input (xbox controller, joystick, keyboard etc.)
 * - video handling (webcam, vision, oculus)
 * 
 * -J
 */

	#ifdef __cplusplus
		extern "C" {
	#endif

		//required for m_module, m_device (take the place of a normal "main" function)
		void configure(void);
		void initialize(void);
		void setup(void);
		void start(void);

	#ifdef __cplusplus
		}
	#endif

	//m_system
	class m_system
	{
		public:
		
		class threading
		{
			public:
			static std::vector<std::thread*> all_running_threads;
			
			static void wait(uint64_t microseconds);
			static void add_thread(std::thread* t);
			static void remove_thread(std::thread* t);
			static void start_threading(void);
			static void wait_on_global_permissive();
			
			static uint64_t now();
			private:
			static m_stoplight global_permissive;
		};
		
		class startup
		{
			public:
			static void configure(void);
			static void initialize(void);
			static void setup(void);
			static void start(void);
		};
		
	};

	//m_module
	class m_module : public m_tag
	{
		public:
			m_stoplight Started{this, "Started"};
			m_stoplight Paused{this, "Paused"};
		
			m_module() {};
			m_module(std::string n); 
			m_module(m_tag* p, std::string n);

			virtual void Register(std::string name) {};
			virtual void Register(m_tag* parent, std::string name) {};
			virtual void start() {};
			virtual void stop() {};
			virtual void pause() {};
			virtual void resume() {};
			virtual void configure() {};
			virtual void wait_for_start(void);
		protected:
		private:
	};
	
	//m_device
	class m_device : public m_module
	{
		public:
			m_device();
			m_device(std::string n);
			m_device(m_tag* p, std::string n);
			virtual void wait_for_start(void);		
		protected:
		private:
	};
	
	//m_run_condition (class template)
	template<class T> class m_run_condition : public m_lockable
	{
		public:
			std::function<bool(void)> test_func;
			pthread_cond_t signaler;
			T* param;
		
			m_run_condition() { pthread_cond_init(&signaler, NULL); };
			m_run_condition(std::function<bool(void)> func) { test_func = func; pthread_cond_init(&signaler, NULL); };
			m_run_condition* operator()(std::function<bool(void)> func) { test_func = func; return this; };
			
			T& wait()
			{
				lock();
				while(test_func() == false)
				{
					pthread_cond_wait(&signaler, getLockHandle());
				}
				unlock();
				return *param;
			};
			
			void notify(T& obj)
			{
				lock();
				param = &obj;
				pthread_cond_broadcast(&signaler);
				unlock();
			};
			
			void notify(T& obj, bool* toChange)
			{
				lock();
				param = &obj;
				*toChange = true;
				pthread_cond_broadcast(&signaler);
				unlock();
			};
	};
	
	//m_run_condition (void template)
	template<> class m_run_condition<void> : public m_lockable
	{
        public:
			std::function<bool(void)> test_func;
			pthread_cond_t signaler;

			m_run_condition() { pthread_cond_init(&signaler, NULL); };
			m_run_condition(std::function<bool(void)> func) { test_func = func; pthread_cond_init(&signaler, NULL); };
			m_run_condition* operator()(std::function<bool(void)> func) { test_func = func; return this; };
			void wait()
			{
				lock();
				while(test_func() == false)
				{
					pthread_cond_wait(&signaler, getLockHandle());
				}
				unlock();
			};
			
			void notify() { lock(); pthread_cond_broadcast(&signaler); unlock(); };
	};

	class m_thread
	{
		public:
			std::string name; //name of the job
			std::thread *run_thread; //the actual thread running
			virtual void start(void) {}; //defualt start function
		protected:
		private:
	};
	
	class m_job_sync;
	
	class m_job_interface : public m_module
	{
	public:
		m_timer* periodicTimer;
		m_stoplight* RunStoplight;
		std::vector<m_job_sync*> synchros;
		
		m_job_interface() {};
		m_job_interface(std::string name) : m_module(name) {};
		m_job_interface(m_tag* parent, std::string name) : m_module(parent, name) {};
		virtual void Register(m_tag* parent, std::string name, uint32_t max_jobs_running, bool reentrent, double task_period) {};
		virtual void start(void) {};
	};
	
	/*
	 * class syncs the execution of two jobs
	 * Makes JobB follow the execution of JobA
	 * JobA cannot run until Job B is complete
	 */
	class m_job_sync : public m_lockable
	{
		public:
			uint64_t jobAcount{0};
			uint64_t jobBcount{0};
			m_job_interface* jobA;
			m_job_interface* jobB;
			pthread_cond_t jobApermissive;
			pthread_cond_t jobBpermissive;
	
			m_job_sync();
			m_job_sync(m_job_interface* A, m_job_interface* B);
	
			void wait(m_job_interface* job);
			void done(m_job_interface* job);
			void waitA();
			void waitB();
	};
	
	//m_worker with two classes
	template<class R, class A> class m_worker : public m_job_interface
	{
		public:
			m_worker() {};
			m_worker(std::string name) : m_job_interface(name) {};
			m_worker(std::function<R(A&)> u_func, std::function<bool(void)> eval_func) { RunOK = eval_func; user_func = u_func;} ;
			m_worker(m_tag* parent, std::string name) { m_job_interface(parent, name); RunOK([](){return true;}); };
			m_worker(m_tag* parent, std::string name, std::function<R(A&)> u_func) 
			{
				m_job_interface(parent, name);
				user_func = u_func;
				RunOK([&](){
					if(this->notified == true)
					{
						this->notified = false;
						return true;
					}
					else
						return false;
				});
			};
			m_worker(std::string name, std::function<R(A&)> u_func) { user_func = u_func; RunOK([](){return true;}); };
			m_worker(std::function<R(A&)> u_func) { user_func = u_func; RunOK([](){return true;}); }; 
			
			void wait_for_start(void) { Running.wait(); };
			void enable(void)
			{
				Running = false;
				std::thread *thread = new std::thread(&m_worker::handle, this);
				thread->detach();
				m_system::threading::add_thread(thread);
				wait_for_start();
			};
			void run_always(std::function<R(A&)> exec)
			{
				user_func = exec;
				RunOK([](){return true;});
			};
			void handle(void)
			{
				while(!exitThread)
				{
					Running = true;
					A& param = RunOK.wait();

					//Running = true;
					try{
						user_func(param);
					}catch(std::exception& e)
					{
						std::cout << "Problem calling thread handle in thread: " << this->name << std::endl;
						exit(-1);
					}
					//Running = false;
				}
				Running = false;
				#ifdef __DEBUG__
					std::cout << "Thread Ending of ID: " << this->name << std::endl;
				#endif
			};
			void run_when(std::function<R(A&)> exec, std::function<bool(A)> eval_func) { user_func = exec; RunOK = eval_func; };
			m_worker& operator=(std::function<R(A&)> func) { user_func = func; return *this; };
			void notify(A& param) { RunOK.notify(param, &this->notified); };
			void operator()(A& param) { notify(param, &this->notified); };
			
		private:
			std::function<R(A&)> user_func; //this is the function that is exectued
			m_run_condition<A> RunOK;
			bool exitThread{false};
			m_stoplight Running{this, "Thread_Running"};
			bool isConditional{true};
			bool notified{true};
	};
	
	//m_worker, no classes
	template<> class m_worker<void, void> : public m_job_interface
	{
		public:
			m_worker() {};
			m_worker(std::string name) : m_job_interface(name) {};
			m_worker(std::function<void(void)> u_func, std::function<bool(void)> eval_func) { RunOK = eval_func; user_func = u_func; };
			m_worker(m_tag* parent, std::string name) : m_job_interface(parent, name) { RunOK([](){return true;}); };
			m_worker(m_tag* parent, std::string name, std::function<void(void)> u_func) : m_job_interface(parent, name) { user_func = u_func; RunOK([](){return true;}); };
			m_worker(m_tag* parent, std::string name, std::function<void(void)> u_func, uint64_t Period, m_stoplight& light) : m_job_interface(parent, name) { user_func = u_func; RunOK([](){return true;}); run_every_when(Period, light); };
			m_worker(std::string name, std::function<void(void)> u_func) : m_job_interface(name) { user_func = u_func; RunOK([](){return true;}); };
		
			void start(void) { enable(); };
			void enable(void)
			{
				if(!Started)
				{
					std::thread *thread = new std::thread(&m_worker::handle, this);
					m_system::threading::add_thread(thread);
					thread->detach();
					if(isPeriodic)
					{
						periodicTimer->run();
					}
				}
			}	
			void handle(void)
			{
				#ifdef __DEBUG__
					std::cout << "Thread Starting of ID: " << this->name << std::endl;
				#endif
				
				Started = true;
				while(!exitThread)
				{
					//if the thread is periodic can we run
					if(!use_stoplight_as_runcond)
						RunOK.wait();
					else
						RunStoplight->wait();
					
					try
					{
						user_func();
					}
					catch(std::exception& e)
					{
						std::cout << "Problem calling thread handle in thread: " << this->name << std::endl;
					}
					if(isPeriodic)
					{
						periodicTimer->wait();
					}
				}
						
				#ifdef __DEBUG__
					std::cout << "Thread Ending of ID: " << this->name << std::endl;
				#endif
			}
			void run_when(std::function<void(void)> exec, std::function<bool(void)> eval_func) { user_func = exec; RunOK = eval_func; };
			void run_when(m_stoplight& stoplight)
			{
				this->use_stoplight_as_runcond = true;
				this->isPeriodic = false;
				this->RunStoplight = &stoplight;
			}
			void run_every_when(std::function<void(void)> exec, uint64_t Period, std::function<bool(void)> eval_func) { user_func = exec; RunOK = eval_func; };
			void run_every_when(uint64_t Period, m_stoplight& stoplight)
			{
				this->use_stoplight_as_runcond = true;
				this->isPeriodic = true;
				this->periodicTimer = new m_timer(this, "PeriodicTimer",Period);
				this->RunStoplight = &stoplight;
			};
			void run_every(uint64_t Period)
			{
				this->use_stoplight_as_runcond = false;
				this->isPeriodic = true;
						
				this->periodicTimer = new m_timer(this, "PeriodicTimer",Period);
				RunOK([]()
				{
					return true;
				});
			};
			void run_always(std::function<void(void)> exec)
			{
				user_func = exec;
				RunOK([]()
				{
					return true;
				});
			};
			m_worker& operator=(std::function<void(void)> func) { user_func = func;	return *this; }
			void notify(void) { RunOK.notify(); };
			
		private:
			std::function<void(void)> user_func; //this is the function that is exectued
			m_run_condition<void> RunOK;
			bool exitThread{false};
			bool isConditional;
			bool isPeriodic{false};
			bool use_stoplight_as_runcond{false};
	};
	//m_worker single class
	template<class R> class m_worker<R, void> : public m_job_interface
	{
		public:
			m_worker() {};
			m_worker(std::string name) : m_job_interface(name) {};
			m_worker(std::function<R(void)> u_func, std::function<bool(void)> eval_func) { RunOK = eval_func; user_func = u_func; };
			m_worker(m_tag* parent, std::string name) : m_job_interface(parent, name) { RunOK([](){return true;}); }; 
			m_worker(m_tag* parent, std::string name, std::function<R(void)> u_func) : m_job_interface(parent, name)
			{ 
				user_func = u_func;
				RunOK([&](){
					if(this->notified == true)
					{
						this->notified = false;
						return true;
					}
					else
						return false;
				});
			}; 
			m_worker(std::string name, std::function<R(void)> u_func) : m_job_interface(name) { user_func = u_func; RunOK([](){return true;}); };
			m_worker(std::function<R(void)> u_func) { 	user_func = u_func; RunOK([](){return true;}); };
			void wait_for_start(void) { Running.wait(); };
			void enable(void)
			{
				Running = false;
				std::thread *thread = new std::thread(&m_worker::handle, this);
				thread->detach();
				m_system::threading::add_thread(thread);
				wait_for_start();
			};
			void run_always(std::function<R(void)> exec) { user_func = exec; RunOK([](){return true;}); };
			void handle(void)
			{
				while(!exitThread)
				{
					Running = true;
					RunOK.wait();

					//Running = true;
					user_func();
					//Running = false;
				}
				Running = false;
				#ifdef __DEBUG__
					std::cout << "Thread Ending of ID: " << this->name << std::endl;
				#endif
			};
			void run_when(std::function<R(void)> exec, std::function<bool(void)> eval_func) { user_func = exec; RunOK = eval_func; }
			m_worker& operator=(std::function<R(void)> func) { user_func = func; return *this; }
			void notify(void) { RunOK.notify(&this->notified); };
		private:
			std::function<R(void)> user_func; //this is the function that is exectued
			m_run_condition<void> RunOK;
			bool exitThread{false};
			m_stoplight Running{this, "Thread_Running"};
			bool isConditional{true};
			bool notified{true};
	};
	
	//m_job with two classes	
	template<typename R, typename A> class m_job : public m_job_interface
	{	
		public:
			m_job() : m_job_interface() {};
			m_job(m_tag* p, std::string n) : m_job_interface(p,n) {};
			m_job(m_tag* p, std::string n, std::function<R(A)> func) : m_job_interface(p,n) { user_func = func; enable(); }
			
			void enable(void)
			{
				lock();
				finished = false;
				run = false;
				exec_thread = new std::thread(&m_job::handle, this);
				unlock();
			};
			void handle(void)
			{
				//set prio and such
				lock();
					
				finished = true;
				while(1)
				{
					unlock();
					run.wait(); //wait for run issues
					//wait for any synchro commands
					lock();
					//execute user and set return
					return_val = user_func(arg);
					finished = true;
					run = false;
				}
			};
			void start()
			{
				lock();
				finished.wait();
				finished = false;
				run = true;
				unlock();
			};
			void start(A arg)
			{
				lock();
				finished.wait();
				finished = false;
				run = true;
				this->arg = arg;
				unlock();
			};
			void operator()(A arg) { start(arg); };
			void exec(std::function<R(A)> f, A args)
			{
				finished.wait();
				lock();
				user_func = f;
				finished = false;
					
				this->arg = args;
					
				run = true;
				unlock();
			};
			R wait_for_finish() { finished.wait(); return(return_val);};
		private:
			std::thread* exec_thread;
			m_stoplight finished;
			m_stoplight run;
			std::function<R(A)> user_func;
			R return_val;
			A arg;
	};
	//m_job with no classes
	template<> class m_job<void, void> : public m_job_interface
	{
		public:
			m_job() : m_job_interface() {};
			m_job(m_tag* p, std::string n) {};
			m_job(m_tag* p, std::string n, std::function<void(void)> func) : m_job_interface(p,n) {user_func = func; enabled();};
			m_job(m_tag* p, std::string n, std::function<void(void)> func, uint64_t period) : m_job_interface(p,n)
			{
				user_func = func;
				//this->periodicTimer = new m_timer([&](){operator()();}
				enabled();
			};
			void enabled(void)
			{
				finished = true;
				run = false;
				exec_thread = new std::thread(&m_job::handle, this);
			}
			void handle(void)
			{
				//set prio and such
				while(1)
					{
					run.wait();
					lock();
					
					//execute user and set return
					user_func();
					finished = true;
					run = false;
					unlock();
				}
			};
			void start()
			{
				//finished.wait();
				//lock();
				//finished = false;
				//run = true;
				//unlock();
			};
			void wait_for_finish() { finished.wait(); };
			void operator()()
			{
				finished.wait();
				lock();
				finished = false;
				run = true;
				unlock();
			};
		private:
			std::thread* exec_thread;
			m_stoplight finished;
			m_stoplight run;
			
			std::function<void(void)> user_func;
	};
	//m_job with a single typename and no classes
	template<typename A> class m_job<void, A> : public m_job_interface
	{
		public:
			void enabled(void)
			{
				lock();
				finished = true;
				run = false;
				exec_thread = new std::thread(&m_job::handle, this);
			}
			void handle(void)
			{
				//set prio and such
				unlock();
				while(1)
					{
					run.wait();
					lock();
					//execute user and set return
					user_func(arg);
					finished = true;
					unlock();
				}
			};
			void start(void)
			{
				lock();
				finished.wait();
				finished = false;
				run = true;
				unlock();
			}
			void start(A arg)
			{
				lock();
				finished.wait();
				finished = false;
				run = true;
				arg = arg;
				unlock();
			}
			void wait_for_finish() { finished.wait(); };
		private:
			std::thread* exec_thread;
			m_stoplight finished;
			m_stoplight run;
			std::function<void(A)> user_func;
			A arg;
	};

	//m_fpga
	class m_fpga
	{
		public:		
			static volatile unsigned *ps_link_base;
			static void link_fpga();
			static bool isFound();
			static void write(uint32_t addr, uint32_t val);
			static uint32_t read(uint32_t addr);
			
			static inline void writeADP(uint32_t addr, uint32_t val);
			static inline uint32_t readADP(uint32_t addr);
			
		private:
			static int fd;
	};

	#ifdef __cplusplus
	extern "C" {
	#endif
	void kmain(void);
	#ifdef __cplusplus
	}
	#endif

#endif