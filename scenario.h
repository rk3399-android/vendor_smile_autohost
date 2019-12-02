#ifndef SCENARIO_H
#define SCENARIO_H

xmlDocPtr parseDoc(char *docname);
int start_scenario(xmlDocPtr doc, char* ip, char* scenariopath, xmlNodePtr properties);

#endif
