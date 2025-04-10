# Projet C2 – Agent de Command & Control

## 🎯 Objectif  
Développer un agent capable de communiquer avec un serveur Command & Control (C2) pour l’exécution de commandes à distance.

## 📅 Planification  

### ✅ Étapes réalisées :  
- **Connexion établie** entre l’agent et le serveur C2  
- **Protocole CSV** implémenté pour l’échange de données  
- **Commandes de base disponibles :**
  - `locate` : récupère la géolocalisation ou des informations de position (à adapter selon le contexte)
  - `sleep` : met en pause l’exécution de l’agent pendant un certain temps
  - `execve` : exécute une commande système via un appel système
  - `cat` : exécute cat sur un fichier
  - `revshell` créé un revshell connecté au client
    
  

### 🛠️ Étapes à venir :
- Amélioration de la robustesse et de la sécurité de la communication C2

## 🚧 À propos  
Ce projet est en cours de développement dans le cadre d'un exercice de cybersécurité.  
Il vise à explorer les techniques de gestion d'agents distants en environnement contrôlé.
