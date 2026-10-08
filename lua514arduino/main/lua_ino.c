/*
* min.c -- a minimal Lua interpreter
* loads stdin only with minimal error handling.
* no interaction, and no standard library, only a "print" function.
*/

#include <stdio.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"

//usb_serial_jtag_write_bytes
//usb_serial_jtag_write_bytes(msg, sizeof(msg), 20 / portTICK_PERIOD_MS);
#include "driver/usb_serial_jtag.h"

//--------------------
//new, added
#define LUA_PROGNAME "lua"
static const char *progname = LUA_PROGNAME;

static void l_message (const char *pname, const char *msg) {
#if 0	
  if (pname) fprintf(stderr, "%s: ", pname);
  fprintf(stderr, "%s\n", msg);
  fflush(stderr);
#else
  char strMsg[256] = {0};
  if (pname) sprintf(strMsg, "%s: ", pname);
  sprintf(strMsg, "%s\n", msg);
  usb_serial_jtag_write_bytes(strMsg, strlen(strMsg), 20 / portTICK_PERIOD_MS);
#endif
}


static int report (lua_State *L, int status) {
  if (status && !lua_isnil(L, -1)) {
    const char *msg = lua_tostring(L, -1);
    if (msg == NULL) msg = "(error object is not a string)";
    l_message(progname, msg);
    lua_pop(L, 1);
  }
  return status;
}
//--------------------


static int print(lua_State *L)
{
  char s[20];
 int n=lua_gettop(L);
 int i;
 for (i=1; i<=n; i++)
 {
  if (i>1) sprintf(s, "\t");
  if (lua_isstring(L,i))
   sprintf(s, "%s",lua_tostring(L,i));
  else if (lua_isnil(L,i))
   sprintf(s, "%s","nil");
  else if (lua_isboolean(L,i))
   sprintf(s, "%s",lua_toboolean(L,i) ? "true" : "false");
  else
   sprintf(s, "%s:%p",luaL_typename(L,i),lua_topointer(L,i));
 }
 //printf("\n");
 //Serial.println(s);
 usb_serial_jtag_write_bytes(s, strlen(s), 20 / portTICK_PERIOD_MS); 
 usb_serial_jtag_write_bytes("\n", 1, 20 / portTICK_PERIOD_MS); 
 return 0;
}

int main__(const char *line)
{
#if 0
  lua_State *L=lua_open();
  lua_register(L,"print",print);
#else
 static lua_State *L = 0;
 if (!L) {
	L=lua_open();
    lua_register(L,"print",print);
 }
#endif
#if 0
 if (luaL_dofile(L,NULL)!=0) fprintf(stderr,"%s\n",lua_tostring(L,-1));
#else
#if 0	
 const char str[] = "print(\"Hello, world!\")";
 int status = luaL_loadbuffer(L, str, sizeof(str) - 1, "main.lua");
#else
 int status = luaL_loadbuffer(L, line, strlen(line), "=stdin");//main.lua");
#endif
 if (status){
   printf("Couldn't load file: %s\n", lua_tostring(L, -1));
  //Couldn't load file: stdin:1: malformed number near '123abc'
 } else {
   status = lua_pcall(L, 0, LUA_MULTRET, 0);
#if 0  
   if (status) {
     printf("Failed to run script: %s\n", lua_tostring(L, -1));
   }
#else
   report(L, status);
#endif



#if 0
if (1) { //if use LUA_MULTRET in lua_pcall
	//see LUA_MULTRET
	if (status == 0/*LUA_OK*/ && lua_gettop(L) > 0) {  /* any result to print? */
		luaL_checkstack(L, LUA_MINSTACK, "too many results to print");
		lua_getglobal(L, "print");
		lua_insert(L, 1);
		if (lua_pcall(L, lua_gettop(L) - 1, 0, 0) != 0/*LUA_OK*/)
			l_message(progname, lua_pushfstring(L,
								   "error calling \"print\" (%s)",
								   lua_tostring(L, -1)));
	}
}
#else
    if (status == 0 && lua_gettop(L) > 0) {  /* any result to print? */
      lua_getglobal(L, "print");
      lua_insert(L, 1);
      if (lua_pcall(L, lua_gettop(L)-1, 0, 0) != 0)
        l_message(progname, lua_pushfstring(L,
                               "error calling " LUA_QL("print") " (%s)",
                               lua_tostring(L, -1)));
    }
#endif	
  

   

 }
 
#endif


  lua_settop(L, 0);  /* clear stack */   
  //fputs("\n", stdout);


#if 0
 lua_close(L);
#endif 
 return 0;
}

// the setup function runs once when you press reset or power the board
void lua_ino_setup(const char *line) {
  //Serial.begin(9600);
  //Serial.println("start-------->");
  main__(line);
}

// the loop function runs over and over again forever
void lua_ino_loop() {

}





