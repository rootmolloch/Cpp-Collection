#include <string>
#include <cstdlib>
#include <iostream>
#include <string.h>

using namespace std;


class Tarot {
	public:
		const string verb() {
			string words[] = {
				"point","precicise","sympathize","echoe","circulate","stopp","educate","number","count","unify","filter",
				"branch","assign","modify",
				"walk","go","type","see","read","talk","laugh","write","pull","push","gett","putt","fight",
				"kill","revive","fuck","die","sing","fly","learn","teach","compute","assign","love","exist",
				"swimm","drink","spitt","splitt","take","give"
			};

			return words[rand() % (sizeof(words) / sizeof(string))];
		}

		const string noun() {
			string words[] = {
				"dott","point","precission","sympathy","echo","circulation","stopp","education","number","one","filter",
				"branch","assignment","modification",
				"rectangle","hunger","color","shape","circle","keyboard","mouth","foot","trouser","eye","screen",
				"lamp","lamb","light","sympathy","circumstance","straw","darkness","horse","radio","sender","receiver",
				"goat","party","racist","racism","Hitler","act","terror","Stalin","Mussolini","democracy","state","land",
				"river","city","angel","country","god","fun","whisky","alcohol","drug","Jew","war","flower","sun","night",
				"hero","moon","earth","world","planet","system","death","life","question","education","sentence","mouse","processor",
				"computer","machine","sabbath","slayer"
			};

			return words[rand() % (sizeof(words) / sizeof(string))];
		}

		const string pastParticiple() {
			string s = verb();
			if(s[s.size() - 1] == 'e') s = s.substr(0,s.size() - 1);
			return s + "ed";
		}

		const string presentParticiple() {
			string s = verb();
			if(s[s.size() - 1] == 'i') s = s.substr(0,s.size() - 1);
			return s + "ing";
		}

		const string adjective() {
			string words[] = {
				"precise","sympathetic","cyclic","single",
				"blue","yellow","guilty","innocent","left","right","hungry","sympathetic","round","rectangular","red",
				"funny","black","white","racistic","dead","lovely","holly","brown","morbid","big","small","forwarded",
				"backwarded"
			};

			string s = words[rand() % (sizeof(words) / sizeof(string))];

			switch(rand() % 4) {
				case 0:
					return s;
				case 1:
					return "more " + s;
				case 2:
					return "most " + s;
				case 3:
					return noun() + "less";
			}

		}

		const string attribute() {
			switch(rand() % 3) {
				case 0:
					return adjective();
				case 1:
					return pastParticiple();
				case 2:
					return presentParticiple();
			}
		}

		const string subject(bool plural) {
			string s = noun();
			if(plural) s = s + "s";
			if(rand() % 4) s = attribute() + " " + s;
			if(rand() % 2) {
				string words[] = {"our","your","their","my","his","her"};
				s = words[rand() % (sizeof(words) / sizeof(string))] + " " + s;
			} else {
				if(plural) {
					string words[] = {"some","many","a lot of","few","a few","plenty of"};
					s = words[rand() % (sizeof(words) / sizeof(string))] + " " + s;
				} else {
					string words[] = {"no","a","one"};
					s = words[rand() % (sizeof(words) / sizeof(string))] + " " + s;
				}
			}

			return s;
		}

		const string question() {
			string words[] = {"Where","What","When","Why","How"};
			string s = words[rand() % (sizeof(words) / sizeof(string))] + " " + s;

			switch(rand() % 4) {
				case 0:
					 return s + " does " + statement(false,false,true);
				case 1:
					 return s + " do " + statement(true,false,true);
				case 2:
					 return s + " did " + statement(false,true,true);
				case 3:
					 return s + " did " + statement(true,true,true);
			}
		}

	public:
		Tarot() {

		}

		const string subject2(bool plural) {
			string s = subject(plural);
			switch(rand() % 3) {
				case 0:
					break;
				case 1:
					s = s + " from " + subject(rand() % 2);
					break;
				case 2:
					s = s + " of " + subject(rand() % 2);
					break;
			}
			return s;
		}

		const string statement(bool plural,bool past,bool q) {
			string s = subject2(plural);

			if(q) {
				s = s + " " + verb();
			} else {
				if(past) {
					s = s + " " + pastParticiple();
				} else {
					if(rand() % 2) {
						if(plural)
							s = s + " " + verb();
						else
							s = s + " " + verb() + "s";
					} else {
						if(plural)
							s = s + " are " + presentParticiple();
						else
							s = s + " is " + presentParticiple();
					}
				}
			}

			s = s + " " + subject2(rand() % 2);
			return s;
		}

		const string get() {
			switch(rand() % 3) {
				case 0:
					return statement(rand() % 2,rand() % 2,false) + ".";
				case 1:
					return statement(rand() % 2,rand() % 2,false) + ", such that " + statement(rand() % 2,rand() % 2,false) + ".";
				case 2:
					return question() + " ?";
			}
		}
};


int main(int argc, char **argv) {
	srand(time(0));

	cout << (new Tarot())->get() << endl;
	
	return 0;
}


