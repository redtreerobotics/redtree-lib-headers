#ifndef RT_INPUT_USER_H
#define RT_INPUT_USER_H

#include <rtr.h>
#include <string>

//joystick
#define JS_EVENT_BUTTON 0x01 // button pressed/released
#define JS_EVENT_AXIS   0x02 // joystick moved
#define JS_EVENT_INIT   0x80 // initial state of device

class JoystickEvent
{
	public:
		unsigned int time;		// timestamp, ms
		short value;			// 0/1 for button, -32768/32767 for axis
		unsigned char type;		// event type
		unsigned char number;	// axis / button number
	
		bool isButton() { return (type & JS_EVENT_BUTTON) != 0; }
		bool isAxis() { return (type & JS_EVENT_AXIS) != 0; }
		bool isInitialState() { return (type & JS_EVENT_INIT) != 0; }
	protected:
	private:
};

class Joystick
{
	public:
		Joystick();
		Joystick(int joystickNumber);
		Joystick(std::string devicePath);
		bool isFound();
		bool sample(JoystickEvent* event);
		~Joystick();
	protected:
	private:
		void openPath(std::string devicePath);
		int _fd;
};

//XBOX
class XBOX_Joystick : public m_device
{
	public:
		using m_device::m_device;
		Joystick* joy_handle;
		
		m_num<int> Poll_Period{this, "Poll_Period"};
		m_num<float> Left_Joystick_X{this, "Left_Joystick_X"};
		m_num<int> Left_Joystick_Y{this, "Left_Joystick_Y"};
		m_num<float> Right_Joystick_X{this, "Right_Joystick_X"};
		m_num<int> Right_Joystick_Y{this, "Right_Joystick_Y"};
		m_num<float> Right_Trigger{this, "Right_Trigger"};
		m_num<float> Left_Trigger{this, "Left_Trigger"};
		
		m_job<void, void> StartCommand{this, "StartCommand", 
		[&]()
		{
			joy_handle = new Joystick("/dev/input/js0");
			if(!joy_handle->isFound())
			{
				std::cout << "NO JOYSTICK" << std::endl;
				Started = false;
			}
			else
			{	
				std::cout << "Started_Joystick" << std::endl;
				Started = true;
			}
		}};
	
		m_worker<void, void> JoyStickPoll{this, "JoystickPoll",
			[&]()
			{
				JoystickEvent event;
				wait_for_start();
				if (joy_handle->sample(&event))
				{
					if (event.isButton())
					{
						//controller(0,4) = (double)event.value;
					}
					else if (event.isAxis())
					{			
						//left-right on left joystick
						if(event.number == 0)
							Left_Joystick_X() = (double)event.value;
						//up-down on left joystick
						if(event.number == 1)
							Left_Joystick_Y() = (double)event.value;
						//left-right on right joystick
						if(event.number == 3)
							Right_Joystick_X() = (double)event.value;
						//up-down on right joystick
						if(event.number == 4)
							Right_Joystick_Y() = (double)event.value;
						if(event.number == 4)
							Right_Trigger() = (double)event.value;
						if(event.number == 5)
							Left_Trigger() = (double)event.value;
					}
				}
			}
		};
	protected:
	private:
};

//general helper functions
float scale_deadband(float gain, float db, float val);

#endif