# Documentation de l'API

---

**_URL de base :_** `http://localhost:4500/api/`

## I. Authentification : `/auth/`

- `/register` Inscription d'un utilisateur

  - **Méthode:** `POST`
  - **Body :**
    ```json
    {
      "username": "string",
      "email": "string",
      "passwd": "string",
      "confirmPasswd": "string"
    }
    ```
  - **Réponse:**
    - **201:**
      ```json
      { "message": "Utilisateur créé avec succès." }
      ```
    - **400:**
      ```json
      { "message": "Les mots de passe ne correspondent pas." }
      ```
      ou
      ```json
      { "message": "Cet utilisateur existe déjà." }
      ```
      ou
      ```json
      { "message": "Erreur lors de la création de l'utilisateur." }
      ```
    - **500:**
      ```json
      { "message": "Erreur serveur." }
      ```

- `/login` Connexion d'un utilisateur

  - **Méthode:** `POST`
  - **Body :**
    ```json
    {
      "email": "string",
      "passwd": "string",
      "rememberMe": boolean
    }
    ```
  - **Réponse:**
    - **200:**
      ```json
      { "token": "jwt_token" }
      ```
      ou
      ```json
      {
        "token": "jwt_token",
        "refreshToken": "refresh_token"
      }
      ```
      si rememberMe est true
    - **400:**
      ```json
      { "message": "Utilisateur non trouvé." }
      ```
      ou
      ```json
      { "message": "Mot de passe incorrect." }
      ```
    - **500:**
      ```json
      { "message": "Erreur serveur." }
      ```
  - **Note:** Le token est également stocké dans un cookie HTTP-only

- `/refresh-token` Renouveler le token d'authentification

  - **Méthode:** `POST`
  - **Auth:** Refresh token (via cookie ou header Authorization)
  - **Réponse:**
    - **200:**
      ```json
      { "newToken": "jwt_token" }
      ```
    - **400:**
      ```json
      { "message": "Token de rafraichissement invalide." }
      ```
    - **401:**
      ```json
      { "message": "Token de rafraichissement manquant." }
      ```

- `/me` Récupérer les informations de l'utilisateur connecté

  - **Méthode:** `GET`
  - **Auth:** JWT Token
  - **Réponse:**
    - **200:** Données de l'utilisateur
    - **404:**
      ```json
      { "message": "Utilisateur non trouvé." }
      ```
    - **500:**
      ```json
      { "message": "Erreur serveur." }
      ```

- `/change-password` Changer le mot de passe de l'utilisateur connecté

  - **Méthode:** `PATCH`
  - **Auth:** JWT Token
  - **Body :**
    ```json
    {
      "oldPasswd": "string",
      "newPasswd": "string",
      "confirmNewPasswd": "string"
    }
    ```
  - **Réponse:**
    - **200:**
      ```json
      { "message": "Mot de passe mis à jour avec succès." }
      ```
    - **400:**
      ```json
      { "message": "Les nouveaux mots de passe ne correspondent pas." }
      ```
      ou
      ```json
      { "message": "Erreur lors de la mise à jour du mot de passe." }
      ```
    - **500:**
      ```json
      { "message": "Erreur serveur." }
      ```

- `/logout` Déconnexion de l'utilisateur

  - **Méthode:** `POST`
  - **Auth:** JWT Token
  - **Réponse:**
    - **200:**
      ```json
      { "message": "Déconnexion réussie." }
      ```
  - **Note:** Les cookies d'authentification sont supprimés

- `/delete-account` Suppression du compte utilisateur
  - **Méthode:** `DELETE`
  - **Auth:** JWT Token
  - **Réponse:**
    - **200:**
      ```json
      { "message": "Compte supprimé avec succès." }
      ```
    - **400:**
      ```json
      { "message": "Erreur lors de la suppression du compte." }
      ```

## II. Utilisateur : `/users/`

- `/statistics` Récupérer les statistiques de l'utilisateur connecté

  - **Méthode:** `GET`
  - **Auth:** JWT Token
  - **Réponse:**
    - **200:** Statistiques de l'utilisateur
    - **404:**
      ```json
      { "message": "Statistiques non trouvées." }
      ```
    - **500:**
      ```json
      { "message": "Erreur serveur." }
      ```

- `/games` Récupérer l'historique des parties de l'utilisateur connecté

  - **Méthode:** `GET`
  - **Auth:** JWT Token
  - **Réponse:**
    - **200:** Liste des parties jouées
    - **404:**
      ```json
      { "message": "Aucune parties jouées pour le moment..." }
      ```
    - **500:**
      ```json
      { "message": "Erreur serveur." }
      ```

- `/social/` Récupérer la liste des relations (amis) de l'utilisateur connecté

  - **Méthode:** `GET`
  - **Auth:** JWT Token
  - **Réponse:**
    - **200:** Liste des amis de l'utilisateur
    - **404:**
      ```json
      { "message": "Aucun ami trouvé." }
      ```
    - **500:**
      ```json
      { "message": "Erreur serveur." }
      ```

- `/social/requests` Récupérer la liste des demandes d'amis reçues par l'utilisateur connecté

  - **Méthode:** `GET`
  - **Auth:** JWT Token
  - **Réponse:**
    - **200:** Liste des demandes d'amis
    - **404:**
      ```json
      { "message": "Aucune demande d'ami trouvée." }
      ```
    - **500:**
      ```json
      { "message": "Erreur serveur." }
      ```

- `/social/requests` Refuser une demande d'ami reçue

  - **Méthode:** `DELETE`
  - **Auth:** JWT Token
  - **Body:**
    ```json
    {
      "username": "string" // Nom d'utilisateur de la personne dont on refuse la demande d'ami
    }
    ```
  - **Réponse:**
    - **200:**
      ```json
      { "message": "Demande d'ami refusée avec succès." }
      ```
    - **400:**
      ```json
      { "message": "Utilisateur non trouvé." }
      ```
      ou
      ```json
      { "message": "Aucune demande d'ami trouvée pour cet utilisateur." }
      ```
    - **500:**
      ```json
      { "message": "Erreur serveur." }
      ```

- `/social/friend` Envoyer une demande d'ami ou accepter une demande d'ami

  - **Méthode:** `POST`
  - **Auth:** JWT Token
  - **Body:**
    ```json
    {
      "username": "string" // Nom d'utilisateur du joueur à ajouter en ami
    }
    ```
  - **Réponse:**
    - **200:**
      ```json
      { "message": "Demande d'ami envoyée avec succès." }
      ```
      ou
      ```json
      { "message": "Demande d'ami acceptée." }
      ```
    - **400:**
      ```json
      { "message": "Utilisateur non trouvé." }
      ```
      ou
      ```json
      { "message": "Vous ne pouvez pas vous ajouter vous-même en ami." }
      ```
      ou
      ```json
      { "message": "Une relation avec cet utilisateur existe déjà." }
      ```
    - **500:**
      ```json
      { "message": "Erreur serveur." }
      ```

- `/social/block` Bloquer un utilisateur

  - **Méthode:** `POST`
  - **Auth:** JWT Token
  - **Body:**
    ```json
    {
      "username": "string" // Nom d'utilisateur du joueur à bloquer
    }
    ```
  - **Réponse:**
    - **200:**
      ```json
      { "message": "Utilisateur bloqué avec succès." }
      ```
    - **400:**
      ```json
      { "message": "Utilisateur non trouvé." }
      ```
      ou
      ```json
      { "message": "Vous ne pouvez pas vous bloquer vous-même." }
      ```
    - **500:**
      ```json
      { "message": "Erreur serveur." }
      ```

- `/social/` Supprimer un ami ou débloquer un utilisateur
  - **Méthode:** `DELETE`
  - **Auth:** JWT Token
  - **Body:**
    ```json
    {
      "username": "string" // Nom d'utilisateur de l'ami à supprimer ou de l'utilisateur à débloquer
    }
    ```
  - **Réponse:**
    - **200:**
      ```json
      { "message": "Ami supprimé avec succès." }
      ```
      ou
      ```json
      { "message": "Utilisateur débloqué avec succès." }
      ```
    - **400:**
      ```json
      { "message": "Utilisateur non trouvé." }
      ```
      ou
      ```json
      { "message": "Aucune relation trouvée avec cet utilisateur." }
      ```
    - **500:**
      ```json
      { "message": "Erreur serveur." }
      ```

## III. Jeux : `/game/`

Cette section sera complétée lorsque les fonctionnalités liées aux jeux seront implémentées.

## IV. Format des réponses

Toutes les réponses sont au format JSON et contiennent:

- En cas de succès: les données demandées ou un message de confirmation
- En cas d'erreur: un objet avec une propriété `message` décrivant l'erreur

## V. Authentification

L'API utilise l'authentification JWT (JSON Web Token).

- Pour les routes protégées, le token doit être inclus dans l'en-tête `Authorization: Bearer [token]` ou via un cookie HTTP-only.
- Le token a une durée de validité d'1 heure.
- Le refresh token, si activé, a une durée de validité de 7 jours.

## VI. Codes d'erreur

- **200:** Succès
- **201:** Ressource créée avec succès
- **400:** Requête invalide (paramètres manquants ou incorrects)
- **401:** Non authentifié
- **403:** Authentifié mais non autorisé
- **404:** Ressource non trouvée
- **500:** Erreur interne du serveur

## VII. WebSockets

Le serveur AGON utilise des WebSockets pour la communication en temps réel, notamment pour les parties de jeu et le chat. Le point de connexion WebSocket est disponible à l'adresse : `ws://localhost:4500/ws`.

### A. Connexion

Pour établir une connexion WebSocket, vous devez fournir deux paramètres obligatoires dans l'URL :

- `token`: Le jeton d'authentification JWT obtenu lors de la connexion HTTP
- `roomId`: L'identifiant de la salle que vous souhaitez rejoindre

**Exemple d'URL de connexion :**

```
ws://localhost:4500/ws?token=votre_token_jwt&roomId=nom_de_la_salle
```

**Réponses possibles :**

- En cas de succès : `Vous avez rejoint la partie : [roomId].`
- En cas d'erreur :
  - `Identification requise.` (Token manquant)
  - `Jeton d'identification invalide.` (Token invalide)
  - `ID de room requis.` (RoomId manquant)
  - `La partie a déjà commencé ou la partie a atteint le nombre maximum de joueurs.` (Room pleine ou partie déjà démarrée)
  - `Vous êtes déjà dans cette room.` (Déjà connecté à cette room)

### B. Communication

Une fois connecté à une WebSocket, vous pouvez envoyer et recevoir des messages selon les formats suivants :

#### Messages de chat

Pour envoyer un message dans le chat de la salle :

```
:CHAT:Votre message ici
```

Les messages de chat reçus auront le format suivant :

```
:CHAT:username (userId) : Contenu du message
```

#### Mise à jour des données de jeu

Pour mettre à jour votre position et d'autres données de jeu :

```
:MAJDATA:{"position":{"x":10,"y":20},"aimDirection":{"x":1,"y":0},"isShooting":true,"isMoving":false}
```

Les champs disponibles sont :

- `position` : Coordonnées x,y du joueur dans l'espace de jeu
- `aimDirection` : Direction visée par le joueur (vecteur normalisé)
- `isShooting` : État de tir du joueur (boolean)
- `isMoving` : État de mouvement du joueur (boolean)

Les mises à jour reçues des autres joueurs auront le format suivant :

```
:MAJDATA:username (userId) : {"position":{"x":10,"y":20},...}
```

### C. Événements système

Le serveur enverra automatiquement les événements suivants :

- Lors de la connexion d'un nouveau joueur : `Le joueur [username] a rejoint la partie !`
- Lors de la déconnexion d'un joueur : `Le joueur [username] a quitté la partie.`

### D. Fermeture de la connexion

La connexion WebSocket est automatiquement fermée lorsque :

- Le client se déconnecte
- Le serveur rencontre une erreur
- Le token d'authentification expire

### E. Limites et contraintes

- Nombre maximum de joueurs par salle : 4
- Les messages sont diffusés à tous les joueurs de la salle à l'exception de l'émetteur
- Un joueur ne peut être connecté qu'à une seule salle à la fois avec le même compte
