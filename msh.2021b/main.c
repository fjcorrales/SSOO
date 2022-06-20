/*-
 * main.c
 * Minishell C source
 * Shows how to use "obtain_order" input interface function.
 *
 * Copyright (c) 1993-2002-2019, Francisco Rosales <frosal@fi.upm.es>
 * Todos los derechos reservados.
 *
 * Publicado bajo Licencia de Proyecto Educativo Práctico
 * <http://laurel.datsi.fi.upm.es/~ssoo/LICENCIA/LPEP>
 *
 * Queda prohibida la difusión total o parcial por cualquier
 * medio del material entregado al alumno para la realización
 * de este proyecto o de cualquier material derivado de este,
 * incluyendo la solución particular que desarrolle el alumno.
 *
 * DO NOT MODIFY ANYTHING OVER THIS LINE
 * THIS FILE IS TO BE MODIFIED
 */
/////////////////////////////////////////LIMPIA LOS COMENTARIOS ANTES DE ENTREGAR!!!!!!!!!!!!!!!!!////////////////
#include <stddef.h>			/* NULL */
#include <stdio.h>			/* setbuf, printf */
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <pwd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/times.h>


extern int obtain_order();	 		/* See parser.y for description */
char cwd[2048];

////////////////////FUNCIONES DE LOS MANDATOS INTERNOS////////////////////
int micd(char *dirname, int nargs);

int miUmask(int nargs, int newMask);

int miLimit(int nargs, char *recurso, int max);

int miSet();
/////////////////////////////////////////////////////////////////////////

int main(void)
{
	char ***argvv = NULL;			//Puntero interesante, matriz de strings, aquí se guarda todo lo que metes en una línea antes de darle al enter en el shell
	int argvc;				//Es un contador que comparamos con el "tamanio" de argvv hasta que "argvv[argvc] != NULL"
	char **argv = NULL;
	int argc;				//igual argvc, para bucles anidados
	char *filev[3] = { NULL, NULL, NULL };	//para el tratamiento de las redirecciones

	int bg;					//indica de forma automatica que hay que ejecutar en el background
	int ret;
	int in;
	int in2;
	int out;
	int out2;
	int outErr;
	int outErr2;
	int contArg;
	int siPipe;
	pid_t pid;

	setbuf(stdout, NULL);			/* Unbuffered */
	setbuf(stdin, NULL);

/*
	Mascaras
*/
	mode_t mask;
	mode_t mask2;

/*
	Tratado de mascaras
*/
	void handler(){
		kill(pid, SIGKILL);
		printf("\n");
	}

	while (1) {
		fprintf(stderr, "%s", "msh> ");	/* Prompt */
		ret = obtain_order(&argvv, filev, &bg);
		if (ret == 0) break;		/* EOF */
		if (ret == -1) continue;	/* Syntax error */
		argvc = ret - 1;		/* Line */
		if (argvc == 0) continue;	/* Empty line */
#if 1
/////////////////TRATAMIENTO DE LAS REDIRECCIONES/////////////////

		//Si tenemos una redireccion de entrada
		if(filev[0]!=NULL){

			if(access(filev[0], F_OK) != 0){
				fprintf(stderr, "ERROR, no se ha podido acceder a la entrada estandar\n");
				continue;
			}

			in = dup(0);
			if((in2 = open(filev[0], O_RDONLY)) == -1) {
				fprintf(stderr, "ERROR, no se ha podido abrir la entrada redireccionada\n");
			}

			dup2(in2, 0);
			close(in2);
		}

		//Si tenemos una redireccion de salida
		if(filev[1] != NULL){

			out = dup(1);
			if((out2 = open(filev[1], O_RDWR | O_CREAT | O_TRUNC, 0666)) == -1){
				fprintf(stderr, "ERROR, no se ha podido abrir el fichero al que se redirige la salida\n");
			}else{
				dup2(out2, 1);
				close(out2);
			}
		}

		//Si tenemos una redireccion de salida de error
		if(filev[2] != NULL){

			outErr = dup(1);
			if((outErr2 = open(filev[1], O_RDWR | O_CREAT | O_TRUNC, 0666)) == -1){
				fprintf(stderr, "ERROR, no se ha podido abrir el fichero al que se redirige la salida de error\n");
			}else{
				dup2(outErr2, 1);
				close(outErr2);
			}
		}

		//para hacer el tratamiento de los pipes simplemente comprobar cunatos argumentos hay en argvv,
		//si hay más de uno, entonces directamente le meto pipe, en "java" if(argvv.size()>1) --> pipe

		int pipes[argvc-1][2];								//array de pipes para saber cuantos necesito? por poner algo
		int cont;
		for(cont = 0 ; argvv[cont]!= NULL ; cont++){
			argv = argvv[cont];								//IMPORTANTE, esto a cada vuelta me guarda en argv el fragmento de la sentencia sin pipe
			for(contArg = 0 ; argv[contArg]!=NULL ; contArg++){}//for para contar el nº de argumentos en un mandato
			if(argvc >= 2){
				if(cont == 0){								//si estoy en el primer mandato, me creo un pipe directamente
					pipe(pipes[0]);							//creo pipes fuera del padre para que tanto padre como hijo tengan acceso al mismo
				}
				pid = fork();

				if(pid==-1){								//error
					fprintf(stderr, "ERROR en el fork\n");
					break;
				}

				////////////PARA PIPES, EL 1-->LECTURA, 0--> ESCRITURA//////////////

				if(pid!=0){//PADRE

					if(argvv[cont+2]!=NULL){					//me creo los pipes "necesarios", solo con vista a 2 mandatos en adelante
						pipe(pipes[cont+1]);
					}

					if(cont > 0){
						close(pipes[cont-1][0]);
						close(pipes[cont-1][1]);

					}
					printf("Papa terminado\n");
				}

				else if(pid == 0){//HIJO
					if(cont == 0){							//si es el primer mandato
						printf("Soy el hijo primer mandato\n");
						close(pipes[cont][0]);
						dup2(pipes[cont][1], 1);
						close(pipes[cont][1]);
					}else if(argvv[cont+1] == NULL){						//si es el ultimo mandato
						printf("Soy el hijo ultimo mandato\n");
						close(pipes[cont-1][1]);
						dup2(pipes[cont-1][0],0);
						close(pipes[cont-1][0]);
					}else{							//si es un mandato entre medias de varios pipes
						printf("Soy el hijo mandato intermedio\n");
						close(pipes[cont-1][1]);
						close(pipes[cont][0]);
						dup2(pipes[cont-1][0],0);
						dup2(pipes[cont][1],1);
						close(pipes[cont-1][0]);
						close(pipes[cont][1]);
					}
					printf("Hijo creado, pipes hechos, antes de entrar a mandatos y execvp\n");
					/* Ejecucion de los mandatos internos y en caso de no haber recibido ninguno de estos
					posterior ejecucion de un execvp para ejecutar el mandato introducido
					REVISAR LAS CONDICIONES DE ENTRADA, PUEDE QUE ALGO MAS SEA NECESARIO BACKGROUND IMPORTANTE
					*/
					if(strstr(argv[0], "cd")!=NULL){
						//TODO
					}else if(strstr(argv[0], "umask")!=NULL){
						//TODO
					}else if(strstr(argv[0], "limit")!=NULL){
						//TODO
					}else if(strstr(argv[0], "set")!=NULL){
						//TODO
					}else{
						//si no le paso ningun mandato interno, hago llamada a execvp para ejecutar el mandato
						printf("Entrando al execvp\n");
						execvp(argv[0], argv);
						fprintf(stderr, "ERROR al ejecutar exec, revisar mandato escrito\n");
						exit(1);
					}
					exit(0);
				}
			}

			/* Mandatos internos en caso de no haber puesto pipes en la linea de comandos
			funcionamiento: if-else anidados para comprobar cual es el mandato pedido
					en caso de no coincidir con ninguno, ejecuta un execvp
			*/
			if(strstr(argv[0], "cd")!=NULL){

				if(argv[1]!=NULL){
					if(micd(argv[1], contArg)!=0){
						fprintf(stderr, "ERROR no se ha podido realizar el cambio de directorio\n");
					}
				}else{
					if(micd("cd", contArg)!=0){
						fprintf(stderr, "ERROR no se ha podido realizar el cambio de directorio\n");
					}
				}

			}else if(strstr(argv[0], "umask")!=NULL){

				if(argv[1] != NULL){
					if(miUmask(contArg, strtol(argv[1], NULL, 8))!=0){
						fprintf(stderr, "ERROR no se ha podido realizar la llamada a umask\n");
					}
				}else{
					if(miUmask(contArg, 0666)!=0){
						fprintf(stderr, "ERROR no se ha podido realizar la llamada a umask\n");
					}
				}

			}else if(strstr(argv[0], "limit")!=NULL){
				if(argv[1]!=NULL && argv[2]!=NULL && contArg == 3){

					//En el caso de tener tanto recurso como nuevo limite

					if(miLimit(contArg, argv[1], atoi(argv[2]))!=0){
						fprintf(stderr, "ERROR no se ha podido realizar la llamada a limit\n");
					}

				}else if(argv[1]!=NULL && contArg == 2){

					//En el caso de solo tener recurso

					if(miLimit(contArg, argv[1], 0)!=0){
						fprintf(stderr, "ERROR no se ha podido realizar la llamada a limit\n");
					}
				}else{
					if(miLimit(1, NULL, 0)!=0){
						fprintf(stderr, "ERROR no se ha podido realizar la llamada a limit\n");
					}
				}

			}else if(strstr(argv[0], "set")!=NULL){
				//TODO
			}else{

				//si no le paso ningun mandato interno, hago llamada a execvp para ejecutar el mandato
				pid = fork();
				if(pid==0){
					execvp(argv[0], argv);
					fprintf(stderr, "ERROR al ejecutar exec, revisar mandato escrito\n");
					exit(1);
				}
				if(pid==-1){
					fprintf(stderr, "ERROR no se ha podido hacer fork para ejecutar el execvp\n");
				}
			}
		}//fin for


/////////////////TRATAMIENTO DE EJECUCION EN BACKGROUND/////////////////

		if(bg){
			printf("[%d]\n", pid);
		}else{
			waitpid(pid, NULL, 0);
		}

#endif

	//Devolvemos las entradas y salidas a su estado original
	if(filev[0]!=NULL){
		dup2(in, 0);
		close(in);
	}
	if(filev[1]!=NULL){
		dup2(out, 1);
		close(out);
	}
	if(filev[2]!=NULL){
		dup2(outErr, 2);
		close(outErr);
	}

	}//fin while
	exit(0);
	return 0;
}//fin main

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////FUNCIONES ASOCIADAS A LOS MANDATOS INTERNOS/////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int micd(char *dirname, int nargs){


	if(nargs == 1){

		/* Como especificado en el documento donde se nos describe la practica
		si el minishell recibe solamente el mandato cd sin argumentos, este
		cambiara el directorio actual al HOME
		*/

		if(chdir(getenv("HOME"))!=0){
			fprintf(stderr, "ERROR no se pudo acceder al directorio HOME\n");
			return -1;
		}
	}

	if(nargs > 2){

		/*
		No puede haber más de un directorio/path a la vez en una llamada al comando cd
		*/

		fprintf(stderr, "ERROR demasiados argumentos\n");
		return -1;
	}

	if(nargs == 2){

		/* Si tenemos solo el madato cd y un directorio/path realizamos una llamada a chdir
		si esta es exitosa, se cambia el directorio y volvemos al shell, en caso contrario
		imprimimos por pantalla el error y volvemos al shell, pero no rompemos el flujo del
		programa ni salimos del shell
		*/

		if(chdir(dirname) != 0){
			fprintf(stderr, "ERROR el directorio especificado no es accesible\n");
			return -1;
		}
	}

	/* Si todo ha ido correctamente, imprimimos por pantalla el path actual
	en el formato especificado en el enunciado y devolvemos 0 para pasar el control de errores
	*/
	printf("%s\n", getcwd(cwd, 2048));
	return 0;
}//fin cd

int miUmask(int nargs, int newMask){

	/* Segun como nos describen el funcionamiento de esta implementacion del mandato interno umask
	tenemos que hacer 2 ifs, si introducimos el mandato solo sin argumentos simplemente enseñaremos por
	pantalla el valor actual de la mascara. Sin embargo si hay un argumento, cambiaremos el valor de la
	mascara al valor pasado como argumento.
	*/

	int prevMask;

	if(nargs == 1){

		/* Dado el funcionamiento de la propia funcion umask, es necesario cambiar la mascara actual para poder saber
		el valor de la misma, por lo que en caso de escribir el mandato umask sin argumentos lo que hare sera
		cambiar el valor de la mascara actual guardando el valor de retorno de la funcion en prevMask y a cotinuacion
		volver a llamar a umask con el valor de prevMask y finalmente imprimir esta misma
		*/

		prevMask = umask(0666);
		umask(prevMask);
		printf("%o\n", prevMask);
		return 0;
	}

	if(nargs<1 || nargs>2){

		/* En cualquiera de estos dos casos hay o demasiados o muy pocos argumentos, por lo que
		debemos mandar un mensaje de error
		*/

		fprintf(stderr, "ERORR fallo en la cantidad de argumentos al realizar umask\n");
		return -1;
	}

	if(nargs == 2){

		/* En caso de tener un argumento al poner el mandato umask, realizaremos una llamada a la funcion umask
		para cambiar la mascara a la deseada pasada por valor e imprimimos la mascara anterior
		*/

		prevMask = umask(newMask);
		printf("%o\n", prevMask);
		return 0;
	}
	return 0;
}//Fin miUmask


int miLimit(int nargs, char *recurso, int max){

	struct rlimit lim;				//Variable necesaria para guardar los valores de los limites

	if(nargs>3){

	/*
	 Si recibo más de 3 elementos, error, la llamada a limit tiene demasiados argumentos
	*/

		fprintf(stderr, "ERROR demasiados argumentos para realizar la llamada a limit");
	}

	if(nargs == 1){

	/*
	Si recibo solo limit, imprimo por pantalla los limites actuales de todos los recursos
	uno por uno  reuso mi variable lim para almacenar los limites y luego imprimir el soft
	*/

		getrlimit(RLIMIT_CPU, &lim);
		printf("%s\t%d\n", "Limite tiempo CPU", (int) lim.rlim_cur);
		getrlimit(RLIMIT_FSIZE, &lim);
		printf("%s\t%d\n", "Limite de tamaño maximo de fichero FSIZE", (int) lim.rlim_cur);
		getrlimit(RLIMIT_DATA, &lim);
		printf("%s\t%d\n", "Limite de tamaño de segmento de datos por proceso DATA", (int) lim.rlim_cur);
		getrlimit(RLIMIT_STACK, &lim);
		printf("%s\t%d\n", "Limite de tamaño de segmento de pila por proceso STACK", (int) lim.rlim_cur);
		getrlimit(RLIMIT_CORE, &lim);
		printf("%s\t%d\n", "Limite de tamaño de fichero CORE", (int) lim.rlim_cur);
		getrlimit(RLIMIT_NOFILE, &lim);
		printf("%s\t%d\n", "Limite del numero de descriptores de fichero NOFILE", (int) lim.rlim_cur);
		return 0;
	}

	if(nargs == 2){

	/* Si recibo solo dos argumentos, suponiendo que el segundo argumento sea un string
	comprobare si es un recurso de los que acepto y en caso afirmativo devolvere su
	limite como especifica el enunciado
	*/

		if(strstr(recurso, "cpu")!=NULL){
			getrlimit(RLIMIT_CPU, &lim);
			printf("%s\t%d\n", "Limite tiempo CPU", (int) lim.rlim_cur);
			return 0;
		}else if(strstr(recurso, "fsize")!=NULL){
			getrlimit(RLIMIT_FSIZE, &lim);
			printf("%s\t%d\n", "Limite de tamaño maximo de fichero FSIZE", (int) lim.rlim_cur);
			return 0;
		}else if(strstr(recurso, "data")!=NULL){
			getrlimit(RLIMIT_DATA, &lim);
			printf("%s\t%d\n", "Limite de tamaño de segmento de datos por proceso DATA", (int) lim.rlim_cur);
			return 0;
		}else if(strstr(recurso, "stack")!=NULL){
			getrlimit(RLIMIT_STACK, &lim);
			printf("%s\t%d\n", "Limite de tamaño de segmento de pila por proceso STACK", (int) lim.rlim_cur);
			return 0;
		}else if(strstr(recurso, "core")!=NULL){
			getrlimit(RLIMIT_CORE, &lim);
			printf("%s\t%d\n", "Limite de tamaño de fichero CORE", (int) lim.rlim_cur);
			return 0;
		}else if(strstr(recurso, "nofile")!=NULL){
			getrlimit(RLIMIT_NOFILE, &lim);
			printf("%s\t%d\n", "Limite del numero de descriptores de fichero NOFILE", (int) lim.rlim_cur);
			return 0;
		}else{
			fprintf(stderr, "ERROR el recurso indicado no es valido, prueba otra vez\n");
			return -1;
		}
	}

	if(nargs==3){

	/* Si tengo tres argumentos, suopniendo que el tercero sea un entero que me representa un limite
	cambiare el limite del recurso que me pasan como me piden en el enunciado, no imprimo nada por pantalla
	*/

		if(strstr(recurso, "cpu")!=NULL){
			getrlimit(RLIMIT_CPU, &lim);
			lim.rlim_cur = max;
			setrlimit(RLIMIT_CPU, &lim);
			return 0;
		}else if(strstr(recurso, "fsize")!=NULL){
			getrlimit(RLIMIT_FSIZE, &lim);
			lim.rlim_cur = max;
			setrlimit(RLIMIT_FSIZE, &lim);
			return 0;
		}else if(strstr(recurso, "data")!=NULL){
			getrlimit(RLIMIT_DATA, &lim);
			lim.rlim_cur = max;
			setrlimit(RLIMIT_DATA, &lim);
			return 0;
		}else if(strstr(recurso, "stack")!=NULL){
			getrlimit(RLIMIT_STACK, &lim);
			lim.rlim_cur = max;
			setrlimit(RLIMIT_STACK, &lim);
			return 0;
		}else if(strstr(recurso, "core")!=NULL){
			getrlimit(RLIMIT_CORE, &lim);
			lim.rlim_cur = max;
			setrlimit(RLIMIT_CORE, &lim);
			return 0;
		}else if(strstr(recurso, "nofile")!=NULL){
			getrlimit(RLIMIT_NOFILE, &lim);
			lim.rlim_cur = max;
			setrlimit(RLIMIT_NOFILE, &lim);
			return 0;
		}else{
			fprintf(stderr, "ERROR el recurso indicado no es valido, prueba otra vez\n");
			return -1;
		}

	}
	return 0;
}//Fin miLimit

int miSet(){
return 0;
}//Fin miSet
