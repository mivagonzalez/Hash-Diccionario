#define _POSIX_C_SOURCE 200809L
#include "hash.h"
#include "lista.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#define TAM_INICIAL 200
#define FACTOR_DE_CRECIMIENTO 3
#define TAMANIO_DE_CRECIMIENTO 10
#define FACTOR_DECRECIENTE 0.25
#define TAMANIO_DE_DECRECIMIENTO 2
#define POS_INICIAL 0
#define AGRANDAR 1
#define ACHICAR 0
#define M 33

typedef struct nodo_hash {
	char* clave;
	void* dato;
} nodo_hash_t;

struct hash {
	lista_t** tabla;
	size_t cantidad;
	size_t tamanio;
	hash_destruir_dato_t destruir_dato;
};

struct hash_iter {
	const hash_t* hash; 
	size_t pos_actual;
	lista_iter_t* iter_lista;
};

/* ******************************************************************
 *                    FUNCIONES AUXILIARES
 * *****************************************************************/


nodo_hash_t* crear_nodo_hash (const char* clave, void* dato) {
	nodo_hash_t* nodo_nuevo = malloc(sizeof(nodo_hash_t));
	if (!nodo_nuevo) return NULL;	
	nodo_nuevo->dato = dato;
	nodo_nuevo->clave = strndup(clave, strlen(clave));
	return nodo_nuevo;
}

size_t hashear(const char* clave, size_t tamanio) {
 	size_t hash = TAM_INICIAL;
    for (size_t i = 0 ; i < strlen(clave); i++){
    	hash = M * hash + clave[i];
    }
    	return hash % tamanio;
}

lista_t** crear_tabla(size_t cantidad) {
	lista_t** tabla_nueva = calloc(cantidad, sizeof(lista_t*));
	if (!tabla_nueva) return NULL;
	size_t i = 0;
	while(i < cantidad) {
		tabla_nueva[i] = lista_crear();
		i++;
	}
	return tabla_nueva;
}


nodo_hash_t* buscar_nodo_en_lista (const hash_t *hash, const char* clave, size_t posicion) {
	lista_t* lista_actual = hash->tabla[posicion];
	lista_iter_t* iter = lista_iter_crear(lista_actual);
	nodo_hash_t* actual = NULL;
	while (!lista_iter_al_final(iter)) {
		actual = lista_iter_ver_actual(iter);
		if (strcmp(actual->clave, clave) == 0) {
			break;
		}
		lista_iter_avanzar(iter);
		actual = NULL;
	}
	lista_iter_destruir(iter);
	return actual;
}	

int buscar_lista_llena(const hash_t* hash, int pos_actual) {
	while(pos_actual < hash->tamanio) {
		if(!lista_esta_vacia(hash->tabla[pos_actual])) {
			return pos_actual;
		}
		pos_actual++;		
	}
	return -1;
}

bool hash_redimensionar(hash_t* hash, size_t capacidad) {
	lista_t** tabla_nueva = crear_tabla(capacidad);
	if (!tabla_nueva) return false;
	for (size_t i = 0; i < hash->tamanio; i++) {
		while(!lista_esta_vacia(hash->tabla[i])){
			nodo_hash_t* aux = lista_borrar_primero(hash->tabla[i]);				
			size_t posicion = hashear(aux->clave, capacidad);
			lista_insertar_ultimo(tabla_nueva[posicion], aux);
		}
		free(hash->tabla[i]);
	}
	free(hash->tabla);
	hash->tamanio = capacidad;
	hash->tabla = tabla_nueva;
	return true;
}


bool comprobar_redimension(hash_t* hash, bool operacion) {
	if(operacion == 1) {
		if((hash->cantidad / hash->tamanio) > FACTOR_DE_CRECIMIENTO){
			if(!hash_redimensionar(hash, hash->tamanio * TAMANIO_DE_CRECIMIENTO)) {
				return false;
			}	
		}	
	}
	if((hash->cantidad / hash->tamanio) < FACTOR_DECRECIENTE) {
		if(!hash_redimensionar(hash, hash->tamanio / TAMANIO_DE_DECRECIMIENTO)) {
			return false;
		}	
	}
	return true;
}

/* ******************************************************************
 *                    PRIMITIVAS DEL HASH
 * *****************************************************************/

hash_t* hash_crear(hash_destruir_dato_t destruir_dato) {
	hash_t* hash = malloc(sizeof(hash_t));
	if (!hash) return NULL;
	lista_t** tabla = crear_tabla(TAM_INICIAL);
	if (!tabla) {
		free(hash);
		return NULL;
	}	
	hash->tabla = tabla;
	hash->cantidad = 0;
	hash->tamanio = TAM_INICIAL;
	hash->destruir_dato = destruir_dato;
	return hash;
}

void* hash_obtener(const hash_t *hash, const char *clave) {
	size_t posicion = hashear(clave, hash->tamanio);
	nodo_hash_t* nodo = buscar_nodo_en_lista(hash, clave, posicion);
	if(!nodo) return NULL;
	return nodo->dato;
}

bool hash_pertenece(const hash_t *hash, const char *clave) {
	size_t posicion = hashear(clave, hash->tamanio);
	nodo_hash_t* nodo_aux = buscar_nodo_en_lista(hash, clave, posicion);
	return nodo_aux ? true : false;
}		

size_t hash_cantidad(const hash_t *hash) {
	return hash->cantidad;
}

bool hash_guardar(hash_t *hash, const char *clave, void *dato) {
	comprobar_redimension(hash, AGRANDAR);
	size_t posicion = hashear(clave, hash->tamanio);
	nodo_hash_t* actual = buscar_nodo_en_lista(hash, clave, posicion);
	if (actual) {
		if (hash->destruir_dato) {
			hash->destruir_dato(actual->dato);
		}
		actual->dato = dato;		
	} else {
		nodo_hash_t* nodo_nuevo = crear_nodo_hash(clave, dato);
		if (!nodo_nuevo) return false;
		if (! lista_insertar_ultimo(hash->tabla[posicion], nodo_nuevo)) return false;
		hash->cantidad++;
	}
	return true;
}

void* hash_borrar(hash_t *hash, const char *clave) { 
	size_t posicion = hashear(clave, hash->tamanio);
	lista_t* lista_actual = hash->tabla[posicion];
	lista_iter_t* iter = lista_iter_crear(lista_actual);
	nodo_hash_t* actual;
	void* dato_aux = NULL;
	while (!lista_iter_al_final(iter)) {
		actual = lista_iter_ver_actual(iter);
		if (strcmp (actual->clave, clave) == 0) {			
			dato_aux = actual->dato;
			lista_iter_borrar(iter);
			free(actual->clave);
			free(actual);
			hash->cantidad--;	
			break;	
		}
		lista_iter_avanzar(iter);
		dato_aux = NULL;
	}
	lista_iter_destruir(iter);
	if (!dato_aux) return NULL;
	if(!comprobar_redimension(hash, ACHICAR)) return NULL;
	return dato_aux;
}

void hash_destruir(hash_t *hash) {
	size_t i = 0;
	while (i < hash->tamanio) {
		while(!lista_esta_vacia(hash->tabla[i])) {
			nodo_hash_t* actual = lista_borrar_primero(hash->tabla[i]);
			if(hash->destruir_dato) {
				hash->destruir_dato(actual->dato);
			}
			free(actual->clave);
			free(actual);
		}
		free(hash->tabla[i]);
		i++;
	}
	free(hash->tabla);
	free(hash);
}

/* ******************************************************************
 *               PRIMITIVAS DEL ITERADOR DEL HASH
 * *****************************************************************/


hash_iter_t* hash_iter_crear(const hash_t *hash) {
	hash_iter_t* iter_nuevo = malloc(sizeof(hash_iter_t));
	if(!iter_nuevo) return NULL;
	int pos_actual = buscar_lista_llena(hash, POS_INICIAL);		
	if(pos_actual >= POS_INICIAL) {
		iter_nuevo->pos_actual = (size_t) pos_actual;
		iter_nuevo->iter_lista = lista_iter_crear(hash->tabla[iter_nuevo->pos_actual]);
	} else {
		iter_nuevo->pos_actual = hash->tamanio;
		iter_nuevo->iter_lista = NULL;
	}
	iter_nuevo->hash = hash;
	return iter_nuevo;
}

bool hash_iter_avanzar(hash_iter_t *iter) {
	if(hash_iter_al_final(iter) || !iter->iter_lista){
		return false;
	}
	lista_iter_avanzar(iter->iter_lista);
	if(!lista_iter_al_final(iter->iter_lista)) {
		return true;
	}
	iter->pos_actual++;
	lista_iter_destruir(iter->iter_lista);
	int pos_actual = buscar_lista_llena(iter->hash, (int)iter->pos_actual);
	if(pos_actual == -1) {
		iter->iter_lista = NULL;
		iter->pos_actual = iter->hash->tamanio;
		return false;
	}
	iter->pos_actual = (size_t) pos_actual;
	iter->iter_lista = lista_iter_crear(iter->hash->tabla[iter->pos_actual]);
	return true;
}

const char* hash_iter_ver_actual(const hash_iter_t *iter) {
	if (hash_iter_al_final(iter) || !iter->iter_lista) {
		return NULL;
	}
	nodo_hash_t* nodo_actual = lista_iter_ver_actual(iter->iter_lista);
	return nodo_actual->clave;
}

bool hash_iter_al_final(const hash_iter_t *iter) {
	return ((iter->pos_actual >= iter->hash->tamanio));
}

void hash_iter_destruir(hash_iter_t* iter) {
	if (iter->iter_lista) {
		lista_iter_destruir(iter->iter_lista);
	}
	free(iter);
}