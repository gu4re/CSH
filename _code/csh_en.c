/* Libraries */
#include <stdio.h>
#include "parser.h"
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <regex.h>
#include <stdbool.h>
#include <sys/stat.h>

/* 	Estructuras de Datos - Lista Enlazada Simple	  *
 * 	Bastante similar a la inicializacion en Pascal 	  */
typedef struct JobsLinkedList {
	int id;	
	pid_t jbid;
	const char * cmd;
	struct JobsLinkedList * sig;

} JobsLinkedList;

/* 	    Variables Globales de la minishell 	  	  */
bool print_prompt = true;
char * mask = NULL;
JobsLinkedList * bgps_list = NULL;
int last_job = -1; 				          // Lo abstraemos y lo mantenemos como variable global para evitarnos futuras busquedas en la LinkedList

/*    Inicializacion de las funciones necesarias de la 	  *
 *		          minishell 			  */
int cd(const char *path, tline* line);
int isOctal(char *oct);
void printf_modded(char buf[]);
void print_umask();
void set_umask(char * mode, tline* line);
int print_job(JobsLinkedList * job);
JobsLinkedList * create_job(pid_t pid, const char * cmd);
int add_job(JobsLinkedList ** head, JobsLinkedList *job);
int check_cjobs(JobsLinkedList ** head, bool show, tline* line);
int fg(int id, JobsLinkedList ** head, tline* line);
void free_mem();
bool one_cmd_bash(tline *line, const char * cmd);
bool one_cmd(tline *line, const char * cmd);
void doline(tline * line, const char * cmd);
void prompt(void);
void handler_msh(void);
int main(void);

/* 			cd function		 	  *
 * 	Aquella que nos permite emular movimientos 	  *
 *		      por directorios  	  		  */
int cd(const char *path, tline* line){

	/* Variables locales del subprograma cd */
	pid_t pid;
        int status, input, output, err;
        char s[100];
	char read_cd[100];				  // Variable que almacena el path leido proveniente de una redireccion

	/* Comprobacion y manejo de redirecciones */
	if (line->redirect_input == NULL)
		input = STDIN_FILENO;
	else if (path != NULL){
		fprintf(stderr,"%s: Error. %s\n",line->commands[0].argv[0],"No es posible aplicar redireccion en este caso.");
		return 1;
	}
	else if ((input = open((char*)line->redirect_input,O_RDONLY)) < 0){
		fprintf(stderr,"%s: Error. %s\n",(char *)line->redirect_input,strerror(errno));
		return 1;
	}

	if (line->redirect_error == NULL)
		err = STDERR_FILENO;
	else if ((err = open(line->redirect_error, O_WRONLY | O_CREAT, 0666)) < 0){
		fprintf(stderr,"%s: Error. %s\n",(char *)line->redirect_error,strerror(errno));
		return 1;
	}
	
	/* Creacion de un subproceso */
        pid = fork();

	/* Comprobacion de errores */
        if (pid < 0){
                fprintf(stderr, "Fallo el fork\n");
        }

	/* Proceso Hijo */
        else if (pid == 0){

		/* Caso en el que tenemos redirecciones */
		if (line->redirect_input != NULL){

			/* Leemos del fichero el posible path */
			read(input, read_cd, 1024);
			close(input);

			read_cd[strcspn(read_cd, "\n")] = 0;

			if (chdir(read_cd) == 0){
                       		printf("%s", "Nuevo directorio: ");
                        	printf("%s\n", getcwd(s, 100));
                        	exit(0);
                	}

			exit(1);
		}

		if (err != STDERR_FILENO) {
			dup2(err, STDOUT_FILENO);
			close(err);
		}

		/* Caso en el que no tenemos redirecciones */

		if(strcmp(path,getenv("HOME")) == 0){
	 		printf("%s", "Cambiando al directorio HOME...\n");
		}

		else{
			printf("%s", "Cambiando de directorio...\n");
                }

                if (chdir(path) == 0){
                        printf("%s", "Nuevo directorio: ");
                        printf("%s\n", getcwd(s, 100));
                        exit(0);
                }

                exit(1);
		
        }
	
	/* Proceso Padre */
        else {
                wait(&status);

                if (WIFEXITED(status) != 0 && WEXITSTATUS(status) != 0){
			
			output = dup(STDOUT_FILENO);

			if (err != STDERR_FILENO) {
				dup2(err, STDOUT_FILENO);
				close(err);
			}

                        fprintf(stderr, "%s: Se ha producido un error al movernos de directorio.\n", line->commands[0].argv[0]);

			if (dup2(output, STDOUT_FILENO) < 0){
				fprintf(stderr,"%s: Error. %s\n",(char *)line->redirect_output,strerror(errno));
				return 1;
			}
			close(output);

                        return 1;
                }
        }
	return 0;
}

/* 			isOctal function 		  *
 * 	Aquella que nos comprueba si la mascara es Octal  *
 *			-1: Si no matchea		  *
 *	0: Si matchea con un 0 delante del estilo 0777	  *
 *		1: Si matchea con algo del estilo 777	  */
int isOctal(char *oct){

	/* 	Variables locales del subprograma isOctal */
	int status[2];
	regex_t re1,re0;

	/* 	Creacion de patrones o regex 		  */
	if (regcomp(&re1, "^[0-7][0-7][0-7]$", REG_NOSUB) != 0)
		return -1;

	if (regcomp(&re0, "^[0?][0-7][0-7][0-7]$", REG_NOSUB) != 0)
                return -1;

	status[1] = regexec(&re1, oct, 0, NULL, 0);
        status[0] = regexec(&re0, oct, 0, NULL, 0);
        regfree(&re1);
        regfree(&re0);
	
	/* 	Comprobamos que se cumpla algun patron 	  */
	if (status[0] != 0 && status[1] != 0)
                return -1;

	if (status[0] == 0)
                return 0;

	return 1;
}

/* 		Auxiliar Printf-umask function 		  *
 * 	Aquella que nos permite mostrar por STDOUT  	  *
 *	 el buffer que guarda los permisos de cada tipo   *
 *	 de usuario. Necesaria ya que utilizamos          *
 *	  el caracter nulo '\0' para "vaciar" el buffer	  */
void printf_modded(char buf[]){

	/* Variables locales del subprograma printf_modded*/
	int i = 0;

	while (i < 3){
		if (buf[i] != '\0')
			printf("%c", buf[i]);
		i = i + 1;
	}
}

/* 		    Printf_umask function 		  *
 * 	Aquella que nos muestra por STDOUT la mascara  	  *
 *	 Bebe de la funcion printf_modded anterior 	  */
void print_umask(){

	/* Variables locales del subprograma print_umask  */
	int i = 0;
	int aux = -1;

	char buf[3];

	/* 	Comprobacion de octal con 0 delante 	  */
	if (isOctal(mask) == 0)
		i = 1;
	
	printf("Octal %s <==> Symbolic ", mask);

	/* Bucle while que itera num a num de la mask     */
	while (i < (int)strlen(mask)){
		aux = (int)(mask[i] - '0');
		
		/* Asignacion rwx con su respectivo octal */
		if (aux - 4 >= 0){
			buf[0] = 'r';
			aux = aux - 4;
		}
		else
			buf[0] = '\0';

		if (aux - 2 >= 0){
			buf[1] = 'w';
			aux = aux - 2;
		}
		else
			buf[1] = '\0';

		if (aux - 1 >= 0){
			buf[2] = 'x';
			aux = aux - 1;
		}
		else
			buf[2] = '\0';

		/* Printf respecto a los permisos del usuario */
		if ((i == 0 && isOctal(mask) == 1) || (i == 1 && isOctal(mask) == 0)){		
			printf("u=");
			printf_modded(buf);			
			printf(",");
		}
		
		/* Printf respecto a los permisos del grupo   */
		if ((i == 1 && isOctal(mask) == 1) || (i == 2 && isOctal(mask) == 0)){
			printf("g=");
			printf_modded(buf);
			printf(",");
		}
		
		/* Printf respecto a los permisos de otros    */
		if ((i == 2 && isOctal(mask) == 1) || (i == 3)){
			printf("o=");
			printf_modded(buf);
			printf("\n");
		}

		i = i + 1;
	}
}

/* 			Set_umask function 		  *
 * 		Aquella que nos guarda la mascara  	  */
void set_umask(char * mode, tline* line){

	/* Variables locales del subprograma set_umask    */
	int p[2];
	int status, input, err;

	pid_t pid;

	char buf[sizeof(mode)];
	char read_mode[sizeof(mode)];

	/*    Posible error en la creacion de la pipe	  */
	if (pipe(p) < 0) {
		fprintf(stderr,"%s: Error. %s\n","pipe",strerror(errno));
		return;
	}

	/* Comprobacion y manejo de redirecciones */
	if (line->redirect_input == NULL)
		input = STDIN_FILENO;
	else if (mode != NULL){
		fprintf(stderr,"%s: Error. %s\n",line->commands[0].argv[0],"No es posible aplicar redireccion en este caso.");
		return;
	}
	else if ((input = open((char*)line->redirect_input,O_RDONLY)) < 0){
		fprintf(stderr,"%s: Error. %s\n",(char *)line->redirect_input,strerror(errno));
		return;
	}

	if (line->redirect_error == NULL)
		err = STDERR_FILENO;
	else if ((err = open(line->redirect_error, O_WRONLY | O_CREAT, 0666)) < 0){
		fprintf(stderr,"%s: Error. %s\n",(char *)line->redirect_error,strerror(errno));
		return;
	}

	/* Creacion de un subproceso */
	pid = fork();

	/* Posible error del pid     */
	if (pid < 0){
                fprintf(stderr, "Fallo el fork\n");
		mask = NULL;
        }

	/* Proceso hijo */
	else if (pid == 0){

		close(p[0]);

		if (line->redirect_input != NULL){
			read(input, read_mode, 1024);
			close(input);

			/* Tratamiento de la linea leída */
			read_mode[strcspn(read_mode, "\n")] = 0;
			
			if (isOctal(read_mode) == -1)
				exit(1);

			/* Le enviamos la mask al padre */
			write(p[1],read_mode,sizeof(read_mode));
			close(p[1]);
			exit(0);
		}

		if (isOctal(mode) == -1)
			exit(1);

		/* Le enviamos la mask al padre */
		write(p[1],mode,sizeof(mode));
		close(p[1]);
		exit(0);
        }

	/* Proceso Padre */
	else{
		close(p[1]);
		wait(&status);

		if (WIFEXITED(status) != 0 && WEXITSTATUS(status) != 0){
                        fprintf(stderr,"%s: Error. %s\n","umask","No se han definido permisos validos");
                        mask = NULL;
                }

		read(p[0],buf,sizeof(mode));
		close(p[0]);
		mask = (char *) malloc(sizeof(char *));
		strcpy(mask, buf);
		
	}
	
}

/* 		Aux job-printf function 		  *
 * 	Aquella que nos muestra por STDOUT un job dado 	  */
int print_job(JobsLinkedList * job) {
	printf("[%d]+ Running \t %s\n", job->id, job->cmd);	
	print_prompt = true;
	return 0;
}

/* 		    New job function 		 	  *
 * 	Aquella que nos permite crear un job para su  	  *
 *		posterior insercion en la lista 	  */
JobsLinkedList * create_job(pid_t pid, const char * cmd){
	JobsLinkedList * job = (JobsLinkedList *)malloc(sizeof(JobsLinkedList *));
	job->jbid = pid;
	job->cmd = cmd;
	job->sig = NULL;

	return job;
}

/* 		    Insert job function 		  *
 * 	Aquella que nos permite insertar un job en la	  *
 *	   lista dinamica para despues mostrarlo 	  */
int add_job(JobsLinkedList ** head, JobsLinkedList *job){
	
	JobsLinkedList * curr = NULL;

	if (*head == NULL){
		job->id = 1;
		*head = malloc(sizeof(JobsLinkedList *));
		*head = job;
		last_job = (*head)->id;
	}
	
	else {
		curr = *head;
		while(curr->sig != NULL)
			curr = curr->sig;

		job->id = (curr->id) + 1;
		curr->sig = job;
		last_job = (curr->sig)->id;
	}

	print_job(job);
		
	return 0;
}

/* 	    Check Current Jobs Main Function  		  *
 * 	Aquella que nos permite conocer el estado	  *
 *	       de los procesos en background        	  */
int check_cjobs(JobsLinkedList ** head, bool show, tline* line){
	
	JobsLinkedList * prev = NULL;
	JobsLinkedList * curr = NULL;
	JobsLinkedList * next = NULL;

	int output, output_aux;

	curr = *head;

	if (curr == NULL)
		return 0;
	
	next = curr->sig;	
	
	while (next != NULL){
		if (waitpid(curr->jbid, NULL, WNOHANG) != 0){

			free((void *)curr->cmd);
			free(curr);
			curr = NULL;

			if (prev == NULL)
				*head = next;
			else
				prev->sig = next;

			curr = next;
			next = next->sig;
		}
		else{
			if (show)
				print_job(curr);
			next = next->sig;
			prev = curr;
			curr = curr->sig;
		}
	}

	if (waitpid(curr->jbid, NULL, WNOHANG) != 0){

		if (prev != NULL)		
			prev->sig = next;
		else
			*head = NULL;

		free((void *)curr->cmd);
		free(curr);
		curr = NULL;


	}
	
	else if (show){

		if (line->redirect_output == NULL){
			print_job(curr);
			return 0;
		}		
		
		fflush(stdout);
		if ((output = open((char*)line->redirect_output,O_WRONLY | O_CREAT, 0666)) < 0){
			fprintf(stderr,"%s: Error. %s\n",(char *)line->redirect_output,strerror(errno));
			return 1;
		}

		if ((output_aux = dup(STDOUT_FILENO)) < 0){
			fprintf(stderr,"%s: Error. %s\n",(char *)line->redirect_output,strerror(errno));
			return 1;
		}

		if (dup2(output, STDOUT_FILENO) < 0){
			fprintf(stderr,"%s: Error. %s\n",(char *)line->redirect_output,strerror(errno));
			return 1;
		}

		close(output);
		print_job(curr);
		fflush(stdout);

		if (dup2(output_aux, STDOUT_FILENO) < 0){
			fprintf(stderr,"%s: Error. %s\n",(char *)line->redirect_output,strerror(errno));
			return 1;
		}

		close(output_aux);
	}

	return 0;

}

/* 		Foreground function 			  *
 *  Aquella que nos permite ejecutar comandos en primer	  *
 * 			plano			  	  */
int fg(int id, JobsLinkedList ** head, tline* line){
	
	pid_t jbid = -1;

	JobsLinkedList * prev = NULL;
	JobsLinkedList * curr = NULL;
	JobsLinkedList * next = NULL;

	char * cmd_cpy = NULL;
	int output, output_aux;

	curr = *head;

	if (curr == NULL){
		if (id == -1)
			fprintf(stderr, "bash: fg: last: no such job\n");
		else
			fprintf(stderr, "bash: fg: %%%d: no such job\n", id);

		if (line->redirect_output != NULL){
			if ((output = open((char*)line->redirect_output,O_WRONLY | O_CREAT, 0666)) < 0){
				fprintf(stderr,"%s: Error. %s\n",(char *)line->redirect_output,strerror(errno));
				return 1;
			}

			close(output);
		}

		return 0;
	}

	next = curr->sig;

	while (next != NULL){
		
		if (id <= curr->id){
			jbid = curr->jbid;
			cmd_cpy = strdup(curr->cmd);
			free((void *)curr->cmd);
			free(curr);
			curr = NULL;

			if (prev == NULL)
				*head = next;
			else
				prev->sig = next;

			curr = next;
			next = next->sig;

			break;
		}
		
		next = next->sig;
		prev = curr;
		curr = curr->sig;
	}

	if (jbid == -1){
		if (curr->id != id){
			fprintf(stderr, "bash: fg: %%%d: no such job\n", id);
			return 0;
		}
		
		jbid = curr->jbid;
		cmd_cpy = strdup(curr->cmd);
		free((void *)curr->cmd);
		free(curr);
		curr = NULL;

		if (prev != NULL)		
			prev->sig = next;
		else
			*head = NULL;

	}

	if (line->redirect_output != NULL){

		fflush(stdout);
		if ((output = open((char*)line->redirect_output,O_WRONLY | O_CREAT, 0666)) < 0){
			fprintf(stderr,"%s: Error. %s\n",(char *)line->redirect_output,strerror(errno));
			return 1;
		}

		if ((output_aux = dup(STDOUT_FILENO)) < 0){
			fprintf(stderr,"%s: Error. %s\n",(char *)line->redirect_output,strerror(errno));
			return 1;
		}

		if (dup2(output, STDOUT_FILENO) < 0){
			fprintf(stderr,"%s: Error. %s\n",(char *)line->redirect_output,strerror(errno));
			return 1;
		}

		close(output);
		printf("%s", cmd_cpy);
		fflush(stdout);

		if (dup2(output_aux, STDOUT_FILENO) < 0){
			fprintf(stderr,"%s: Error. %s\n",(char *)line->redirect_output,strerror(errno));
			return 1;
		}

		close(output_aux);
	}

	waitpid(jbid, NULL, 0);
	return 0;
}

/* 		      Free Memory function 		  *
 *    Aquella que nos permite liberar la memoria usada	  *
 *         	durante el tiempo de ejecucion  	  */
void free_mem(){

	JobsLinkedList * next = NULL;
	JobsLinkedList ** head = NULL;
	
	head = &bgps_list;
	
	if (*head != NULL){
		check_cjobs(&bgps_list, false, NULL);

		if (*head != NULL){
			next = (*head)->sig;

			while (next != NULL){
				free((void *)bgps_list->cmd);
				free(*head);
				*head = NULL;

				*head = next;
				next = next->sig;

			}

			free((void *)bgps_list->cmd);
			free(*head);
			*head = NULL;

		}

		printf("%s", "Liberada la memoria asociada a la lista de jobs...\n");
	}
	
	if (mask != NULL){
		free(mask);
		mask = NULL;
		printf("%s", "Liberada la memoria asociada a la mascara de permisos...\n");
	}
}

/* 		Call to one-cmd-bash function 		  *
 * Aquella que nos permite ejecutar un comando externo 	  *
 * 			de bash  			  */
bool one_cmd_bash(tline *line, const char * cmd){
	/* Local variables */
	int status, output, input, err;
	pid_t pid;

	if (line->redirect_output == NULL)
		output = STDOUT_FILENO;
	else if((output = open((char*)line->redirect_output,O_WRONLY | O_CREAT, 0666)) < 0){
                fprintf(stderr,"%s: Error. %s\n",(char *)line->redirect_output,strerror(errno));
		return false;
	}
	if (line->redirect_input == NULL)
		input = STDIN_FILENO;
	else if((input = open((char*)line->redirect_input,O_RDONLY)) < 0){
		fprintf(stderr,"%s: Error. %s\n",(char *)line->redirect_input,strerror(errno));
		return false;
	}
	if (line->redirect_error == NULL)
		err = STDERR_FILENO;
	else if ((err = open(line->redirect_error, O_WRONLY | O_CREAT, 0666)) < 0){
		fprintf(stderr,"%s: Error. %s\n",(char *)line->redirect_error,strerror(errno));
		return false;
	}
	// New subprocess
	pid = fork();
	// Checking errors
	if (pid < 0){
		fprintf(stderr,"Fallo el fork()");
		return false;
	}
	/* Children process */
	else if (pid == 0){

		/* Controlling SIGINT handler in children process */
		if (line->background){
			if (signal(SIGINT, SIG_IGN) == SIG_ERR)
				fprintf(stderr, "Fallo en la inicializacion del handler.");
		}
		else
			if (signal(SIGINT, SIG_DFL) == SIG_ERR)
				fprintf(stderr, "Fallo en la inicializacion del handler.");
		if (output != STDOUT_FILENO) {
			dup2(output, STDOUT_FILENO);
			close(output);
		}
		if (input != STDIN_FILENO) {
			dup2(input, STDIN_FILENO);
			close(input);
		}
		if (err != STDERR_FILENO) {
			dup2(err, STDOUT_FILENO);
			close(err);
		}
		execvp(line->commands[0].argv[0], line->commands[0].argv);
		exit(1);
	}
	// Parent process
	else{
		if (!line->background){
			wait(&status);
			if (WIFEXITED(status) != 0 && WEXITSTATUS(status) != 0){
                fprintf(stderr, "bash: No ha sido posible ejecutar el comando.\n");
				return false;
			}
		}
		else
			add_job(&bgps_list, create_job(pid, strdup(cmd)));	
	}
	return true;
}

/* 		             one_command function 	    		    *
 * That function that process a command from our functions  *
 *              or from bash default commands               */
bool one_cmd(tline *line, const char * cmd){
    // Local variables
	int err;

	if(strcmp(line->commands[0].argv[0], "cd") == 0){
		switch(line->commands[0].argc){
			case 1:
				if (line->redirect_input == NULL)					
					cd(getenv("HOME"), line);
				else
					cd(NULL, line);
				break;
			case 2:
				cd(line->commands[0].argv[1], line);
				break;
			default:
				if (line->redirect_error == NULL){
					fprintf(stderr, "%s: Numero de argumentos incorrecto\n",line->commands[0].argv[0]);
					return true;
				}
				else if ((err = open(line->redirect_error, O_WRONLY | O_CREAT, 0666)) < 0){
					fprintf(stderr,"%s: Error. %s\n",(char *)line->redirect_error,strerror(errno));
					return false;
				}
				if (err != STDERR_FILENO) {
					dup2(err, STDERR_FILENO);
					close(err);
				}
				fprintf(stderr, "%s: Numero de argumentos incorrecto\n",line->commands[0].argv[0]);
		}
		return true;
	}
	else if(strcmp(line->commands[0].argv[0], "exit") == 0){
		// Freeing memory before exiting
		free_mem();
		exit(0);
	}
	else if(strcmp(line->commands[0].argv[0], "umask") == 0){
		switch(line->commands[0].argc){
			case 1:
                if (line->redirect_input == NULL){
                    if (mask == NULL){
                        fprintf(stderr, "%s: Mascara aun no asignada\n",line->commands[0].argv[0]);
						break;
					}
					print_umask();
				}
				else
					set_umask(NULL, line);
                break;
            case 2:
                set_umask((char *)line->commands[0].argv[1], line);
                break;
            default:
                fprintf(stderr, "%s: Numero de argumentos incorrecto\n",line->commands[0].argv[0]);
		}
		return true;
	}
	else if(strcmp(line->commands[0].argv[0], "jobs") == 0){
		switch(line->commands[0].argc){
			case 1:
				check_cjobs(&bgps_list, true, line);
				break;
			default:
                fprintf(stderr, "%s: Numero de argumentos incorrecto\n",line->commands[0].argv[0]);
		}
		return true;
    }
	else if(strcmp(line->commands[0].argv[0], "fg") == 0){
		switch (line->commands[0].argc){
			case 1:
				fg(last_job, &bgps_list, line);
				break;
			case 2:
				fg(atoi(line->commands[0].argv[1]), &bgps_list, line);
				break;
			default:
				fprintf(stderr, "%s: Numero de argumentos incorrecto\n",line->commands[0].argv[0]);
		}
		return true;
    }
	return one_cmd_bash(line, cmd);
}

/* 		                doline function 		             *
 *    Process the line looking for the number of cmd passed  */
void doline(tline * line, const char * cmd){
	/* Local variables */
	int i;
	int input, output, err;
	int p[2];
	pid_t pid;

	/* 0 or null command */
	if (line->ncommands == 0)
		return;
	// case: 1 command from bash or own commands
	if (line->ncommands == 1 && one_cmd(line, cmd))
		return;
    // case: More than 1 command
	if (line->ncommands > 1){
		if (line->redirect_input == NULL)
			input = STDIN_FILENO;
		else if((input = open((char*)line->redirect_input,O_RDONLY)) < 0){
			fprintf(stderr,"%s: Error. %s\n",(char *)line->redirect_input,strerror(errno));
			return;
		}
		if (line->redirect_error == NULL)
			err = STDERR_FILENO;
		else if ((err = open(line->redirect_error, O_WRONLY | O_CREAT, 0666)) < 0){
			fprintf(stderr,"%s: Error. %s\n",(char *)line->redirect_error,strerror(errno));
			return;
		}
		// for loop for 2 or more commands with pipes
		for (i = 0; i < line->ncommands; i++){
			if (i < line->ncommands - 1) {
				if (pipe(p) < 0) {
					fprintf(stderr,"%s: Error. %s\n","pipe",strerror(errno));
					return;
				}
				output = p[1];
			}
			else{
				if (line->redirect_output == NULL)
					output = STDOUT_FILENO;
				else if((output = open((char*)line->redirect_output,O_WRONLY | O_CREAT, 0666)) < 0){
                    fprintf(stderr,"%s: Error. %s\n",(char *)line->redirect_output,strerror(errno));
					return;
				}
			}
			pid = fork();
			if (pid == 0){
				if (line->background){
					if (signal(SIGINT, SIG_IGN) == SIG_ERR)
					fprintf(stderr, "Fallo en la inicializacion del handler.");
				}
				else{
					if (signal(SIGINT, SIG_DFL) == SIG_ERR)
						fprintf(stderr, "Fallo en la inicializacion del handler.");
				}
				if (input != STDIN_FILENO) {
					dup2(input, STDIN_FILENO);
					close(input);
				}
				if (output != STDOUT_FILENO) {
					dup2(output, STDOUT_FILENO);
					close(output);
				}
				if (err != STDERR_FILENO) {
					dup2(err, STDERR_FILENO);
					close(err);
				}
				execvp(line->commands[i].argv[0], line->commands[i].argv);
				fprintf(stderr, "bash: No ha sido posible ejecutar el comando.\n");
				return;
			}
			input = p[0];
			if (i < line->ncommands - 1)			
				close(output);	
		}
		if (err != STDERR_FILENO)
			close(err);
		if (line->background)
			add_job(&bgps_list, create_job(pid, strdup(cmd)));
		else
			wait(NULL);
	}
}

/* 		             Prompt 			  */
void prompt(void){
	if (print_prompt){
		usleep(2500);	        // Time sleep to synchronize
		printf("%s","msh> ");
		usleep(2500);           // Time sleep to synchronize
	}
}

/* 		     Handler_MiniShell 		  	  *
 *    Handler for Ctrl+C signal SIGINT	  */
void handler_msh(void){
	signal(SIGINT, (void *)handler_msh);
	printf("\nmsh> ");
	fflush(stdout);
	print_prompt = false;
}

/* 		         Main function 		  	  */
int main(void){
	/* Local variables */
	char buf[1024];
	tline *line;

	/* SIGINT handler */
	if (signal(SIGINT, (void *)handler_msh) == SIG_ERR)
		fprintf(stderr, "Fallo en la inicializacion del handler.");
	prompt();
	/* While loop until STDIN is closed */
        while (fgets(buf, 1024, stdin) != NULL) {
		    /* Tokenize the line that 'fgets' got */
            line = tokenize(buf);
		    /* Modifying the read line */
		    buf[strcspn(buf, "\n")] = 0;
		    doline(line, buf);
		    prompt();
		    print_prompt = true;
        }	
        return 0;
}
