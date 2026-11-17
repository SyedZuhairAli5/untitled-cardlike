#include "raylib.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
using namespace std;

//ENUMS AND STRUCTS
enum Suit{
    NONE,
    HEARTS,
    DIAMONDS,
    SPADES,
    CLUBS
};

enum CombatantType{
    PLAYER,
    ENEMY
};

struct Card{
    int rank;  //2-10 normal number ranks |11, 12, 13 are the jack, queen and king| 14 = ace
    Suit suit;

    Card(int r = 0, Suit s = NONE):rank(r), suit(s){}  //default and parameterized constructor

    void displayCard(){
        cout << "Rank = " << rank << "| Suit = ";
        switch(suit){
            case HEARTS: cout << "HEARTS" << endl; break;
            case DIAMONDS: cout << "DIAMONDS" << endl; break;
            case SPADES: cout << "SPADES" << endl; break;
            case CLUBS: cout << "CLUBS" << endl; break;
            default: break;
        }
    }
};

//CLASSES
class DrawPile{  //queue
    Card cards[52];
    int front, rear, count;

    public:
        DrawPile(): front(0), rear(-1), count(0){}

        void initialize(Card deck[]){
            front = 0;
            rear = 51;
            count = 52;

            for(int i=0; i<52; i++){
                cards[i] = deck[i];
            }
        }

        void enqueue(Card c){
            if(count == 52) return;
            rear = (rear + 1)%52;
            cards[rear] = c;
            count++;
        }

        Card dequeue(){
            if(count == 0)return Card(); //rank = 0, suit = NONE

            Card c = cards[front];
            front = (front + 1)%52;
            count--;
            return c; 
        }

        bool isEmpty(){return count == 0;}
        int getCount(){return count;}

        void displayDrawPile(){
            for(int i=0; i<count; i++){
                int index = (i+1)%52; //since we're dealing with a circular queue
                cards[index].displayCard();
            }
        }
};  

class DiscardPile{
    Card cards[52];
    int top;

    public:
        DiscardPile(): top(-1){}

        void push(Card c){
            if(top == 51) return;
            cards[++top] = c;
        }

        Card pop(){
            if(top == -1) return Card();
            return cards[top--];
        }

        bool isEmpty(){return top == -1;}
        int getCount(){return top+1;}
};

class Hand{
    Card hand[8];
    bool cardSelected[8];
    string name;
    int count;

    public:
        Hand(string n = " "): name(n), count(0){
            for(int i=0; i<8; i++){
                cardSelected[i] = false;
            }
        }

        void addCard(Card c){
            if(count == 8) return;
            hand[count] = c;
            cardSelected[count] = false;
            count++;
        }

        Card getCard(int index){  //like pop 
            if(index < 0 || index >= count) return Card();
            return hand[index];
        }

        void toggleSelection(int index){
            if(index < 0 || index >= count) return;

            if(cardSelected[index]) cardSelected[index] = false;
            else if(getSelectedCount() < 5) cardSelected[index] = true;
        }

        Card removeCard(int index){
            if(index < 0 || index >= count) return Card();
            Card temp = hand[index];

            for(int i=index; i<count-1; i++){
                hand[i] = hand[i+1];   //shift every card after the removed card one space back 
                cardSelected[i] = cardSelected[i+1];
            }
            count--;
            cardSelected[count] = false;
            return temp;
        }

        int getCount(){return count;}
        bool isEmpty(){return count == 0;}
        bool isFull(){return count == 8;}

        bool isSelected(int index){
            if(index < 0 || index >= count) return false;
            return cardSelected[index];
        }

        int getSelectedCount(){
            int total = 0;

            for(int i=0; i<count; i++){
                if(cardSelected[i]) total++;
            }

            return total;
        }

        void displayHand(){
            cout << name << "'s Hand\n-----------------\n";
            for(int i=0; i<count; i++){
                hand[i].displayCard(); 
            }
        }
};

class Combatant{
    DrawPile drawPile;
    DiscardPile discPile;
    Hand hand;
    CombatantType ct;
    int health, maxHealth;

    public:
        Combatant(CombatantType c, int h); //constructor defined outside the class

        void drawTillFull();
        void playSelected();

        void takeDamage(int dmg){
            health -= dmg;
            if(dmg >= health) health = 0;
        }

        //lifesteal implementation
        void heal(int amnt){
            health += amnt;
            if(health >= maxHealth) health = maxHealth;
        }

        Hand& getHand(){return hand;}
        DrawPile& getDP(){return drawPile;}
        DiscardPile& getDiscard(){return discPile;}
        int getHealth(){return health;}
        int getMaxHealth(){return maxHealth;}
        CombatantType getType(){return ct;}
};

//HELPER FUNCTIONS
void createDeck(Card deck[]){
    int index = 0;

    for(int suit = HEARTS; suit <= CLUBS; suit++){
        for(int rank = 2; rank <=14; rank++){
            deck[index++] = Card(rank, (Suit)suit); //(Suit)suit to convert suit(currently an int) into the Suit enum used for the card's constructor
        }
    }
}

void shuffleDeck(Card deck[], int size){ 
    for(int i=size-1; i>0; i--){  //going backwards
        int j  = rand()% (i+1);
        Card temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}

//constructor
Combatant::Combatant(CombatantType c, int h): ct(c), health(h), maxHealth(h), hand(c == PLAYER?"PLAYER":"ENEMY"){
    Card deck[52];

    createDeck(deck);
    shuffleDeck(deck, 52);
    drawPile.initialize(deck);
}

void recycleDiscardPile(DrawPile &d, DiscardPile &discard){
    if(!d.isEmpty() || discard.isEmpty()) return;  //don't do anything if cards are still present in the DP OR if no cards in DISCARD

    Card temp[52];
    int count = 0;

    while(!discard.isEmpty()){
        temp[count++] = discard.pop();
    }

    shuffleDeck(temp, count);  //shuffle the temp card deck
    for(int i=0; i<count; i++){
        d.enqueue(temp[i]);
    }
}

void fillHandTill8(Hand &h, DrawPile &d, DiscardPile &discard){
    while(h.getCount() < 8){
        if(d.isEmpty()){
            if(discard.isEmpty()) break;
            recycleDiscardPile(d, discard);
        }

        h.addCard(d.dequeue());
    }
}

void Combatant::drawTillFull(){
    fillHandTill8(hand, drawPile, discPile);
}

string getRankName(int rank){
    switch(rank){
        case 11: return "J"; 
        case 12: return "Q";
        case 13: return "K";
        case 14: return "A";
        default: return to_string(rank);
    }
}

string getSuit(Suit suit){
    switch(suit){
        case HEARTS: return "HEARTS";
        case DIAMONDS: return "DIAMONDS";
        case SPADES: return "SPADES";
        case CLUBS: return "CLUBS";
        default: return "";
    }
}

void handlePlayerHandInput(Hand &h){
    const int CARD_WIDTH = 110;
    const int CARD_HEIGHT = 150;
    const int CARD_GAP = 10;
    if(h.getCount() == 0) return;

    int totalWidth = h.getCount() * CARD_WIDTH + (h.getCount() - 1) * CARD_GAP;
    int startX = (1280 - totalWidth)/2;
    int baseY = 520;

    Vector2 mousePos = GetMousePosition();

    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
        for(int i=0; i<h.getCount(); i++){
            int x = startX + i * (CARD_WIDTH + CARD_GAP);
            int y = baseY;
            if(h.isSelected(i)) y-= 20;  //lift selected cards

            Rectangle cardRect = {(float)x, (float)y, (float)CARD_WIDTH, (float)CARD_HEIGHT};

            if(CheckCollisionPointRec(mousePos, cardRect)){  //check whether this card was clicked or not
                h.toggleSelection(i);
                break;
            }
        }
    }
}

void drawPlayerHand(Hand &h){
    const int CARD_WIDTH = 110;
    const int CARD_HEIGHT = 150;
    const int CARD_GAP = 10;
    if(h.getCount() == 0) return;

    int totalWidth = h.getCount() * CARD_WIDTH + (h.getCount() - 1) * CARD_GAP;
    int startX = (1280 - totalWidth)/2;
    int baseY = 520;

    Vector2 mousePos = GetMousePosition();

    for(int i=0; i<h.getCount(); i++){  //display each card in hand
        int x = startX + i * (CARD_WIDTH + CARD_GAP);
        int y = baseY;

        if(h.isSelected(i)) y-= 20;  //lift selected cards

        Rectangle cardRect = {(float)x, (float)y, (float)CARD_WIDTH, (float)CARD_HEIGHT};
   
        if(h.isSelected(i)) DrawRectangleRec(cardRect, SKYBLUE);
        else DrawRectangleRec(cardRect, WHITE);
        DrawRectangleLinesEx(cardRect, 2, BLACK);  //outline

        Card c = h.getCard(i);
        string rank = getRankName(c.rank);
        string suit = getSuit(c.suit);

        Color suitColor = (c.suit == HEARTS || c.suit == DIAMONDS)? RED:BLACK;

        DrawText(rank.c_str(), x+10, y+10, 25, BLACK);
        DrawText(suit.c_str(), x+10, y+55, 18, suitColor);
    }
}

void drawPlayButton(Rectangle playButton){
    Vector2 mousePos = GetMousePosition();
    bool hovering = CheckCollisionPointRec(mousePos, playButton);

    Color buttonColour = hovering? DARKBLUE: BLUE;
    DrawRectangleRec(playButton, buttonColour);
    DrawRectangleLinesEx(playButton, 2, BLACK);
    DrawText("PLAY", playButton.x + 62, playButton.y + 15, 25, WHITE);
}

void playSelectedCards(Hand &h, DiscardPile &d){
    for(int i = h.getCount() - 1; i >= 0; i--){  //checking cards from right to left
        if(h.isSelected(i)){
            Card playerCard = h.removeCard(i);
            d.push(playerCard);
        }
    }
}

void Combatant::playSelected(){
    playSelectedCards(hand, discPile);
}

//MAIN FUNCTION
int main(){
    InitWindow(1280, 720, "Untitled Cardlike");
    SetTargetFPS(60);
    srand(time(0)); //seeding the randomizer

    //hands and filling hands
    Combatant player(PLAYER, 100);
    player.getDP().displayDrawPile();
    player.drawTillFull();

    while (!WindowShouldClose()){
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
            Rectangle playButton = {550, 420, 180, 55};
            Vector2 mousePos = GetMousePosition();

            if(CheckCollisionPointRec(mousePos, playButton)){
                if(player.getHand().getSelectedCount() > 0){
                    player.playSelected();
                    player.drawTillFull();
                }
            }
            else handlePlayerHandInput(player.getHand());
        }

        BeginDrawing();

        ClearBackground(DARKGREEN);
        DrawText("PLAYER HAND", 550, 475, 20, WHITE);
        drawPlayButton({550, 420, 180, 55});
        drawPlayerHand(player.getHand());
        
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
