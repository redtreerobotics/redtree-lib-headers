#ifndef RT_TYPES_H
#define RT_TYPES_H

	#include <map>			//std::map
	#include <vector>		//std::vector
	#include <functional>	//std::function
	#include <iostream>		//std::cout
	#include <rtr.h>

	//m_lock
	class m_lock
	{
		public:
			m_lock();
			virtual void lock();
			virtual void unlock();
			pthread_mutex_t* getLockHandle();
			pthread_mutexattr_t lock_attributes;
			pthread_mutex_t internal_lock;
	};
	class m_lockable : public m_lock
	{
		public:
	};

	//m_tag
	class m_option_interface 
	{
		public:
		virtual void apply(m_option_interface* ptr){};
		virtual void execute(void){};
		virtual std::string getName(void){return "";};
	};
	#define m_option_list std::vector<m_option_interface*>
	class m_subscription_interface;
	class m_tag : public m_lockable
	{
		public:
			m_option_list tag_options;
			m_option_list applied_options;
			std::string name;
			bool needs_deletion;
			
			m_tag(std::string name);
			m_tag(m_tag* parent, std::string n);
			m_tag(m_tag* parent, std::string n, m_option_list option_list);
			m_tag(std::string n, m_option_list option_list);
			m_tag() {};
			   
			virtual void initialize(){};
			virtual void setup(){};
			virtual void configure(){};
			virtual void start(){};
			virtual void apply_options(void);
			virtual void subscribe(m_subscription_interface& subs) {};
			static std::vector<std::string> split(const std::string &s, char delim);
			static std::map<std::string, void*> Parse_String_Args(std::string fmt, va_list args);
			
			/* Required to be implemented by child tags for serialization over the network */
			virtual int getSerializedSize(void);
			virtual char * tag_to_buffer(int * length);
			virtual void buffer_to_tag(char * buffer);
			
			virtual ~m_tag();
				
		protected:
		private:
	};
	template<class T> class m_external : public m_tag
	{
		public:
			T* target;
			m_external(){};
			m_external(m_tag* parent, std::string name) : m_tag(parent, name){};
			m_external(std::string name) : m_tag(name){};
			void reference(T& t) { target = &t; };
			T& operator ()(void) { return *target; };
			m_external& operator=(T& t) { reference(t); return *this; }
	};
	#define m_option_handle(N) [&](N arg)
	template<class T> class m_option : public m_tag, m_option_interface
	{
		public:
			using m_tag::m_tag;
			T val;
			std::function<void(T)> option_func;
			virtual std::string getName(void){ return name; };
			m_option(m_tag* parent, std::string name, T default_val, std::function<void(T)> option_application) : m_tag(parent, name)
				{ val = default_val; option_func = option_application; };
			m_option() {};
			m_option(std::string name, T value) : m_tag(name) { val = value; }
			virtual void apply(m_option_interface* ptr) { m_option<T> *temp = dynamic_cast<m_option<T>*>(ptr); val = temp->val;	}
			virtual void execute(void) { option_func(this->val); };
	};
	template<class T> m_option_interface* m_optionset(std::string alias, T value)
	{
		m_option_interface *temp = new m_option<T>(alias, value);
		return temp;
	} 
		
	//m_subscription
	class m_subscription_interface
	{
		
		public:
			//a function that is called when the notify method is called.
			virtual void notify(void){};
			virtual void notify(m_tag *t){};
	};
	template<class U> class m_subscribable 
	{
		public:
			std::vector<m_subscription_interface*> subscribers; 
			virtual void subscribe(m_subscription_interface& subs) { subscribers.push_back(&subs); };
			virtual void subscribe(m_subscription_interface* subs) { subscribers.push_back(subs); };
			virtual void subscribe_ptr(m_subscription_interface* subs) { subscribers.push_back(subs); };
			virtual void unsubscribe(void) {};
			virtual void notify_subscribers(U* val)
			{
				uint32_t x;
				for(x = 0; x<subscribers.size(); x++)
				{
					m_subscription_interface* s = subscribers.at(x);
					s->notify(val); //notify the subscribers that something has changed
				}
			}
	};
	template<class G, class T, void (T::*Q)(G&)> class m_value_subscription : public m_subscription_interface
	{
		public:
			T* parent_module;
			G* subscription_value;
			m_value_subscription() { parent_module = NULL; subscription_value = new G(); }
			m_value_subscription(T* p) { parent_module = p; subscription_value = new G(); }
			virtual void notify(m_tag *t)
			{
				G* temp = dynamic_cast<G*>(t);
				
				if(subscription_value != NULL)
				{
					delete subscription_value;
				}
				
				subscription_value =  temp->copy();
				(parent_module->*Q)(*subscription_value);
			};
		protected:
		private:
	};
	/*
	 * This type of subscription will simply call a function. The function
	 * takes an m_tag pointer as its argument - the m_tag pointer is the tag
	 * that fired when it was changed. Note, more complicated shit is above
	 * for cases where functions are members of objects / classes.
	 * -J
	 */
	class m_value_simple_subscription : public m_subscription_interface
	{
			void (*callback)(m_tag *) = NULL;
			public:
					m_value_simple_subscription(){};
					m_value_simple_subscription(void (*f)(m_tag *t)) { callback = f; };
			
			virtual void notify(m_tag *t)
			{
					if(callback != NULL)
					{
							callback(t);
					}
			}
	};
		
	//m_num
	enum
	{
		M_INT,
		M_UINT,
		M_FLOAT,
		M_DOUBLE,
	};
	
	template<class T> class m_num : public m_tag
	{
		private:
			T Value;
		
		public:
			m_num() : m_tag("") {};
			m_num(std::string n) : m_tag(n) {};
			m_num(m_tag* p, std::string n) : m_tag(p, n) {};
		
			inline T& operator *(void){return Value;};
			inline m_num<T>& operator=(T val){Value = val; return *this;};
			inline operator T(){return Value;};
			inline T& operator()(){return Value;};
		
			char * tag_to_buffer(int * length);
			void buffer_to_tag(char * buffer);
		
			int getType() { return typeid(Value).hash_code(); };
	};

	//m_yesno
	#define yes true
	#define no false
	class  m_yesno_value_abstract;
	class m_yesno : public m_tag, public m_subscribable<m_yesno>
	{
		public:
			using m_tag::m_tag;
			m_yesno();
			m_yesno(m_tag* parent, std::string name);
			m_yesno(m_yesno* c);
			
			void Register(m_tag *parent, std::string name, bool initial_value);
			
			operator bool();
			void set(bool val);
			bool get(void);
			
			m_yesno& operator=(bool val);
			m_yesno& operator=(int val);
			m_yesno& operator=(m_yesno& val);
			
			m_yesno_value_abstract* value;
	};
	class m_yesno_value_abstract
	{
		public:
			m_yesno_value_abstract();
			m_yesno_value_abstract(m_yesno* parent);
			
		
		
			virtual void set(bool arg);
			virtual bool get();
				virtual ~m_yesno_value_abstract();
			
			bool val;
			m_yesno* owner;
	};
	class m_yesno_value_real : public m_yesno_value_abstract
	{
		public:
			m_yesno_value_real();
			m_yesno_value_real(m_yesno* parent, bool initial);
			m_yesno_value_real(m_yesno* parent);
			~m_yesno_value_real();
	};
	class m_yesno_value_link : public m_yesno_value_abstract
	{
		public:
			m_yesno_value_link();
			m_yesno_value_link(m_yesno* parent, m_yesno* target);
		
			virtual void set(bool arg);
			virtual bool get();
		
			~m_yesno_value_link();
			
			m_yesno* linkedto;
	};

	//m_stoplight
	class m_stoplight : public m_tag
	{		
		public:
		
		m_stoplight(); 
		m_stoplight(m_tag* parent, std::string name);
		m_stoplight(std::string name, bool val);
		
		//no longer inline - obsolete in modern compilers: http://stackoverflow.com/questions/3992980/c-inline-member-function-in-cpp-file
		void wait();
		m_stoplight& operator=(bool val);
		void change(bool val);
		void wait_for(bool val);
		operator bool();
		
		private:
			bool value{false};
			pthread_cond_t signaler;
	};
	class m_stoplight_subscription : public m_subscription_interface
	{
		public:
			std::function<bool (void)> conditional_function;
			m_stoplight_subscription() {};
			m_stoplight_subscription(m_stoplight* l, std::function<bool (void)> f);
			virtual void notify();
			virtual void notify(m_tag*);
		private:
			m_stoplight* light;
	};


	/*
	 * m_blob
	 */

	/*
	 * m_string
	 */
	 
	/*
	 * m_matrix
	 */

	/*
	 * m_stream
	 */
	 
	/*
	 * m_pipe
	 */

	/*
	 * m_yesno
	 */
	 
	//m_timer
	class m_timer : public m_tag
	{
		public:
			m_timer();
			m_timer(m_tag* parent, std::string name, double p);
			
			m_num<double> Period{"Period"};
			m_num<double> Ticks{"Ticks"};
			m_num<int> RunCount{"RunCount"};
			m_yesno AutoReload{"AutoReload"};
			m_stoplight TimerEnable{"TimerEnable", false};
		
			std::function<void(void)> trigger;
			pthread_cond_t timedOut;
		
			void handle(void);
			void wait(void);
			void run(void);
	};
	
	//m_tagdb
	enum {
		M_DIGITAL,
		M_NUMERIC,
		M_NUM,
		M_BLOB,
		M_STRING,
		M_IMAGE,
		M_AUDIO,
		M_VIDEO,
		M_USER,
	};

#endif