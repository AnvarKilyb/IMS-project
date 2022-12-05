#include "simlib.h"
#include <ctime>

 
const int MINUTE = 60;
 
const int HOUR = 60*MINUTE;
 
const int DAY = 24*HOUR;
 
const int MONTH = 30*DAY;
 
const int YEAR = 12*MONTH;

 
const double SIMULATION_TIME = 1*YEAR;

 
const int FERRY_CAPACITY = 300;
 
const int CHECK_IN = 12;
 
const int OPEN_CHECK_IN = 4;
 
const int ARRIVAL_GATES = 4;
 
const int DEPARTURE_GATES = 9;
 
const int LOADING_PEOPLE = 4;

 
const int OPEN_CHECK = 1; 
 
const int SECURITY = 1;  
 
const int ID_CONTROL = 4;
 
const int CLOTHES = 4;
 
const int SEC_POSTS = 2;
 
const int INDIVIDUAL_CHECK = 1;


 
int opened_checkin = 0;
 
int gen_checkin = 0;
 
int opened_sec = 0;
 
int gen_sec = 0;
 
int opened_arrival = ARRIVAL_GATES;
 
int opened_departure = 0;
 
int loading_people = 0;

 
bool isDay = false;
 
bool isSeason = true;

 
Facility checkin[CHECK_IN];
int used[CHECK_IN];
 
Queue q_checkin;

 
Facility Runway;
 
Facility gate_arrival[ARRIVAL_GATES];
 
Queue q_apron;

 
Queue q_departure_gate[DEPARTURE_GATES];
 
Queue q_departure_gate_ferry;
 
Queue q_loading_people;
 
Queue q_loading;

 
Store id_control("Pasy", ID_CONTROL);
 
Store sec_posts("sec_posts", SEC_POSTS);
 
Store individual_check("INDIVIDUAL_CHECK", INDIVIDUAL_CHECK);
 
Store dress("dress",CLOTHES);
 
Store undress("undress",CLOTHES);


 
Queue q_sec_lobby;


 
Histogram time_passengers("Total passengers time in system",  1000, 10*MINUTE, 20);
Histogram time_ferries("Total ferry-boats time in system",1200, 10*MINUTE, 20);
Histogram time_security_checks("Full time of security checks in system", 0, 10*MINUTE, 15);

int amnt_in_people = 0;
int amnt_out_people = 0;
int amnt_in_ferry = 0;
int amnt_out_ferry = 0;


Stat sec_front;
int people_on_sec=0;


 
 
class Loading_passengers : public Event {


	void Behavior() {

		 
		while( ! q_loading.Empty()) {

			bool loaded = false;

			 
			for(int i=0; i < DEPARTURE_GATES; i++) {

				if(q_departure_gate[i].Length() >= FERRY_CAPACITY) {

					loaded = true;
					for(int j=0; j < FERRY_CAPACITY; j++) {
						q_departure_gate[i].GetFirst()->Activate();
					}
					break;
				}
			}

			 
			if(loaded) {
				(q_loading.GetFirst())->Activate();

			 
			} else {
				break;
			}


		}

		Activate(Time+5);
	}
};


 
class Passenger : public Process {

	void Behavior() {

		double arrival = Time;
		amnt_in_people++;

		 
		if(Random() <= 0.35) {

			 
			if(opened_checkin == 0) {
				q_checkin.Insert(this);
				Passivate();
			}

			int idx=0;
			for (int a=0; a < opened_checkin; a++) {
				if (checkin[a].QueueLen() < checkin[idx].QueueLen())
					idx=a;
			}

			used[idx]++;

			 
			Seize(checkin[idx]);
			Wait(Uniform(20,50));
			Release(checkin[idx]);
		}

		 
		if(opened_sec == 0) {
			q_sec_lobby.Insert(this);
			Passivate();
		}

		 
		double pp = Time;

		people_on_sec++;
		sec_front(people_on_sec);

		

		 
		Enter(dress, 1);
		Wait(30);
		Leave(dress);

		 
		for(int i=0; searching() && (i < 3); i++);

		 
		Enter(undress, 1);
		Wait(30);
		Leave(undress);

		people_on_sec--;
		time_security_checks(Time-pp);

		 
		Wait(Uniform(10,15));

		int idx=0;
		for (int i = 0; i < opened_departure; ++i) {
			if(q_departure_gate[i].Length() > q_departure_gate[idx].Length()) {
				idx = i;
			}
		}
		q_departure_gate[idx].Insert(this);
		Passivate();

		time_passengers(Time-arrival);
		amnt_out_people++;
	}

	 
	 
	bool searching() {

		Enter(sec_posts, 1);
		Wait(15);
		Leave(sec_posts);

		 
		if(Random() <= 0.3) {

			 
			if(Random() < 0.5) {
				Enter(individual_check, 1);
				Wait(15);
				Leave(individual_check);
				return false;
			}

			return true;
		} else {
			return false;
		}

	}

};


 
class To_open : public Process {

	void Behavior() {

		 
		if(opened_checkin < CHECK_IN) {
			opened_checkin += OPEN_CHECK_IN;
		}
		gen_checkin += OPEN_CHECK_IN;

		 
		while (!q_checkin.Empty()) {
			Process *tmp = (Process *)q_checkin.GetFirst();
			tmp->Activate();
		}


		 
		if(opened_sec < SECURITY) {
			opened_sec += OPEN_CHECK;
		}
		gen_sec += OPEN_CHECK;

		 
		while (!q_sec_lobby.Empty()) {
			Process *tmp = (Process *)q_sec_lobby.GetFirst();
			tmp->Activate();
		}
	}
};

 
class To_close : public Process {

	void Behavior() {

		if(gen_checkin <= CHECK_IN) {
			opened_checkin -= OPEN_CHECK_IN;
		}
		gen_checkin -= OPEN_CHECK_IN;


		if(gen_sec <= SECURITY) {
			opened_sec -= OPEN_CHECK;
		}
		gen_sec -= OPEN_CHECK;

	}
};

 
 
class Unloading : public Process {
	int number_gateway;
	void Behavior(){

		opened_arrival--;

		 
		Seize(gate_arrival[number_gateway]);
		Wait (Uniform((15*MINUTE),(20*MINUTE)));
		Release(gate_arrival[number_gateway]);
		
		opened_arrival++;

		 
		if (!q_apron.Empty()) {
			Process *tmp = (Process *)q_apron.GetFirst();
			tmp->Activate();
		}
		

		Wait (Uniform((10*MINUTE),(20*MINUTE)));
	}
	
	public:
		Unloading (int arrival_gate) : number_gateway(arrival_gate) {
			Activate();
		}
};

 
class Generator_passengers : public Event {

	int passengers;
	int gen;

	void Behavior() {

		(new Passenger)->Activate();
		gen++;

		if(gen < passengers) {
			Activate(Time + Uniform(15,40));

		}
		
	}

	public:
		Generator_passengers(int zak) : passengers(zak) {
			gen = 0;
			Activate();
		}
};

class Plane : public Process {


	void Behavior() {

		amnt_in_ferry++;
		
		 
		(new Generator_passengers(FERRY_CAPACITY))->Activate();
		Wait(1*HOUR);


		 
		(new To_open)->Activate();

		 
		if(opened_departure >= DEPARTURE_GATES) {
			q_departure_gate_ferry.Insert(this);
			Passivate();
		}

		opened_departure++;
		
		 
		Wait(1*HOUR);

		 
		Seize(Runway);
		Wait (2*MINUTE);
		Release(Runway);

		 
		double arrival_time = Time;
		
		 
		if (opened_arrival==0) {
			q_apron.Insert(this);
			Passivate();
		}
		 
		int idx=0;
		for (int a=0; a < opened_arrival; a++) {
			if (gate_arrival[a].QueueLen() < gate_arrival[idx].QueueLen()) {
				idx=a;
			}
		}
		 
		(new Unloading(idx))->Activate();
		
		 
		Wait(Uniform(3*MINUTE, 6*MINUTE));
		
		 
		Wait(Uniform(10*MINUTE,30*MINUTE));

		 
		if(loading_people >= LOADING_PEOPLE) {
			q_loading_people.Insert(this);
			Passivate();
		}

		loading_people++;
		q_loading.Insert(this);
		Passivate();

		 
		Wait(2*MINUTE);
		
		 
		(new To_close)->Activate();
		opened_departure--;

		 
		if( ! q_departure_gate_ferry.Empty()) {
			q_departure_gate_ferry.GetFirst()->Activate();
		}

		 
		loading_people--;
		 
		if( ! q_loading_people.Empty()){
			q_loading_people.GetFirst()->Activate();
		}

		 
		Wait(Uniform(5*MINUTE, 10*MINUTE));

		 
		Seize(Runway);
		Wait (2*MINUTE);
		Release(Runway);

		time_ferries(Time-arrival_time);
		amnt_out_ferry++;
		
	}

};

 
class Generator_ferries : public Event {

	 
	int max_sails_for_day;
	 
	int gen_ferries_for_day;

	void Behavior() {

		 
		if(isSeason) {

			if(isDay && (gen_ferries_for_day < max_sails_for_day)) {
				(new Plane)->Activate();
				gen_ferries_for_day++;

				double next;

				if(Random() < 0.20) {
					next = Uniform(60*MINUTE, 80*MINUTE);

				} else if(Random() < 0.70){
					next = Uniform(30*MINUTE, 60*MINUTE);
				} else {
					next = Uniform(5*MINUTE, 30*MINUTE);
				}

				Activate(Time + next);
			}

		 
		} else {

			if(isDay && (gen_ferries_for_day < max_sails_for_day)) {
				(new Plane)->Activate();
				gen_ferries_for_day++;

				Activate(Time + Uniform(HOUR, 6*HOUR));
			}
		}
	}

	public:
		Generator_ferries(){

			if(isSeason) {
				max_sails_for_day = Uniform(12,20);
			} else {
				max_sails_for_day = Uniform(4,8);
			}
			gen_ferries_for_day = 0;

			Activate();
		}
};

 
class Generator_day : public Event {

	void Behavior() {

		if(isDay) {
			isDay = false;
			Activate(Time + (8*HOUR));

		 
		} else {
			isDay = true;
			(new Generator_ferries())->Activate();
			Activate(Time + (16*HOUR));
		}
	}
};

 
class Generator_season : public Event {

	void Behavior() {

		if(isSeason) {
			isSeason = false;
			Activate(Time + (6.5*MONTH));
		} else {
			isSeason = true;
			Activate(Time + (5.5*MONTH));
		}
	}
};

 
int main() {

	RandomSeed(time(NULL));
	 
	Init(0,SIMULATION_TIME);   

	
	(new Generator_season)->Activate();
	(new Generator_day)->Activate();	
	(new Loading_passengers())->Activate();
	
	 
	Run();

	 
	time_ferries.Output();
	time_security_checks.Output();
	time_passengers.Output();


}
