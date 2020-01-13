#include <iostream>
#include <stdio.h>  
#include <string.h>   //strlen  
#include <stdlib.h>  
#include <errno.h>  
#include <unistd.h>   //close  
#include <arpa/inet.h>    //close  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros  
#include <vector>

using namespace std;

#define PORT 5752 

// Forward Declarations
class Client;

class Gamepiece
{
public:
  Gamepiece( int fg, char c, int d);
  ~Gamepiece();
  // Copy Constructor
  Gamepiece( const Gamepiece& );
  void setOwner( int i ){ owner=i; };
  int getOwner(){ return owner; };
  void setCharacter( char c ){ character=c; };
  void setBackground( bool b );
  void setForeground( bool b );
  
  string * getBackground(){ return ( new string( *background ) ); };
  string * getForeground(){ return ( new string( *foreground ) ); };
  string * getReset(){ return ( new string( *reset ) ); };
  string * getCharacter(){ return ( new string( 1, character ) ); };
  void made_move(){ move_number++; };
  int getMoveNumber(){ return move_number; };
  int getDirection(){ return direction; };
protected:
  friend ostream &operator << (ostream &out, const Gamepiece &gp); 
  
private:
  char character;
  string * background;
  string * foreground;
  string * reset;
  int move_number;
  int owner;// this user id of the owner of the piece
  int direction;
};

Gamepiece::Gamepiece(const Gamepiece& gp) 
{
  character=gp.character;
  background=new string( gp.background->c_str() );
  foreground=new string( gp.foreground->c_str() );
  reset=new string( gp.reset->c_str() );
  move_number = gp.move_number;
  owner=gp.owner;
  direction=gp.direction;
} 

void Gamepiece::setForeground( bool b )
{
  delete foreground;
  if( b )
    {
      foreground = new string("\033[1;30m" );
    }
  else
    {
      foreground = new string("\033[1;31m" );
    }
  

}
void Gamepiece::setBackground( bool b )
{
  delete background;
  if( b )
    {
      background = new string("\033[48;5;253m" );
    }
  else
    {
      background = new string("\033[48;5;244m" );
    }
}

Gamepiece::~Gamepiece()
{
  if( background != NULL ) delete background;
  if( foreground != NULL ) delete foreground;
  if( reset != NULL ) delete reset;
  
}
ostream & operator << (ostream &out, const Gamepiece &gp) 
{
  out << *gp.background << *gp.foreground << " " << gp.character << " " << *gp.reset;
  return out;
}

Gamepiece::Gamepiece( int fg, char c, int d=1)
{
  character=c;
  background= new string("\033[1;47m\033[1;34m" );
  move_number=0;
  foreground= new string("\033[1;" + std::to_string(fg) + "m");
  reset= new string("\033[1;0m");
  direction=d;
}


class Chess
{
public:
  Chess(){};
  Chess( int a, int b );
  
  void displayBoard(int sd);
  int isPromotion();
  void displayBoard();
  bool move( string * s, int uid);
  void setPlayer1( Client * c ){ player1 = c; };
  void setPlayer2( Client * c ){ player2 = c; };
  int getColumn(int p){ return p%8; };
  int getRow(int p){ return p/8; };
  int getIndex( int r, int c ){ return r*8+c; };

  bool isValidMove( int m );
  bool isCheck(int who_to_check);
  bool isSameOwner(int a, int b);
  bool isOnBoard(int a);
  
  void validMoves( int position );
  void setPlayer1UserID( int i ){ player1uid=i; };
  void setPlayer2UserUID( int i ){ player2uid=i; };
  Client *  getPlayer1(){ return player1; };
  Client *  getPlayer2(){ return player2; };
  void initBackgrounds();
  void initPieces();
  
  bool isOccupied( int i );
  bool isCorrectOwner( int p );
private:
  vector<Gamepiece*> board;
  int player1uid;
  int player2uid;
  int player1sd;
  int player2sd;
  int whosturn;
  Client * player1;
  Client * player2;
  
  vector<int> valid_moves;
};

int Chess::isPromotion()
{
  for( int i=0; i<8; i++ )
    {
      cerr << board[i]->getCharacter()->at(0) << endl;
      if( board[i]->getCharacter()->at(0) == 'P' )
	{
	  cerr << "There is a pawn at " << i << endl;
	  return i;
	}
    }
  for( int i=56; i<64; i++ )
    {
      cerr << board[i]->getCharacter()->at(0) << endl;
      if( board[i]->getCharacter()->at(0) == 'P' )
	{
	  cerr << "There is a pawn at " << i << endl;
	  return i;
	}
    }
  return -1;
}

bool Chess::isCorrectOwner( int p )
{
  return( board[p]->getOwner() == whosturn );

}
bool Chess::isOnBoard(int a)
{
  return (a>=0 && a<64); 
}

bool Chess::isSameOwner(int a, int b)
{
  return ( board[a]->getOwner() == board[b]->getOwner() );
}

bool Chess::isCheck(int who_to_check)
{
#ifdef DEBUG
  cerr << "Checking to see if a king is in check." << endl;
#endif
  int kings_index_in_vector;
  // first, find the king in question
  for( int i=0; i<63; i++ )
    {
      if( board[i]->getOwner() == who_to_check && board[i]->getCharacter()->at(0) == '@' )
	{
	  kings_index_in_vector = i;
#ifdef DEBUG
	  cerr << "found the king in question at " << kings_index_in_vector << endl;
#endif
	  break;
	}
    }
  // second, check every piece on the board
  // and get their valid moves
  
  for( int i=0; i<64; i++ )
    {
      if( i!=kings_index_in_vector )
	{
#ifdef DEBUG
	  cerr << "checking space (" << i << ") for valid moves... ";
#endif
	  validMoves(i);
#ifdef DEBUG
	  cerr << "found " << valid_moves.size() << endl;
#endif
	}
    }
#ifdef DEBUG
  int tmp_size=valid_moves.size();
#endif
  
  // remove any duplicates (not REALLY necessary)
  sort( valid_moves.begin(), valid_moves.end() );
  valid_moves.erase( unique( valid_moves.begin(), valid_moves.end() ), valid_moves.end() );
#ifdef DEBUG
  cerr << "Removed " << tmp_size - valid_moves.size() << " moves that were duplicates." << endl;
#endif
  
  // third check to see if any of those valid moves
  // will be where the king is currently.
  for( int j=0; j<valid_moves.size(); j++ )
    {
#ifdef DEBUG
      cerr << "checking to see if (" << valid_moves[j] <<  ") IS the king's position" << endl;
#endif
      if( valid_moves[j] == kings_index_in_vector )
	{
#ifdef DEBUG
	  cerr << "KING IS IN CHECK!" << endl;
#endif
	  return true;
	}
    }
#ifdef DEBUG
  cerr << "King is NOT in Check" << endl;
#endif
  valid_moves.erase( valid_moves.begin(), valid_moves.begin()+valid_moves.size() );
  return false;
}

bool Chess::isValidMove(int m)
{
#ifdef DEBUG
  cerr << "There are " << valid_moves.size() << " valid moves for " << m << endl;
#endif
  bool retVal=false;
  for( int i=0; i<valid_moves.size(); i++ )
    {
      int x=player1uid;
      if( player1uid==whosturn ) x=player2uid;
      
      if( valid_moves[i]==m && !isCheck(x))
	{
	  retVal=true;
#ifdef DEBUG
	  cerr << m << " is a valid move" << endl;
#endif
	  break;
	}
    }
  return retVal;
}
void Chess::initBackgrounds()
{
  for( int i=0; i<board.size(); i++ )
    {
      int row = getRow(i);
      if(  ( ((row)%2 == 0 ) && ( (i)%2==0 ) ) || ( ((row)%2==1) && ((i)%2==1)))
	{
	  board[i]->setBackground(false);
	}
      else
	{
	  board[i]->setBackground(true);
	}
      
      if( i == (8*row+7) )
	{
	  row++;
	}     
    }
}

void Chess::initPieces()
{
  for( int i=0; i<board.size(); i++ )
    {
      delete board[i];
    }
  for( int i=board.size()-1; i>0; i-- )
    {
      board.erase(board.begin()+1);
    }
  int black_colour=30;
  int white_colour=34;
  
  // initialize board
  board.push_back( new Gamepiece( white_colour, 'R' ) );
  board.push_back( new Gamepiece( white_colour, 'K' ) );
  board.push_back( new Gamepiece( white_colour, 'B' ) );
  board.push_back( new Gamepiece( white_colour, 'Q' ) );
  board.push_back( new Gamepiece( white_colour, '@' ) );
  board.push_back( new Gamepiece( white_colour, 'B' ) );
  board.push_back( new Gamepiece( white_colour, 'K' ) );
  board.push_back( new Gamepiece( white_colour, 'R' ) );
  
  for( int i=0; i<8; i++ ) board.push_back( new Gamepiece( white_colour, 'P' ) );
  
  // empty spaces
  for( int i=0; i<32; i++ ) board.push_back( new Gamepiece( 30, ' ', 0 ) );
  
  for( int i=0; i<8; i++ ) board.push_back( new Gamepiece( 30, 'P', -1 ) );
  
  board.push_back( new Gamepiece( 30, 'R', -1 ) );
  board.push_back( new Gamepiece( 30, 'K', -1 ) );
  board.push_back( new Gamepiece( 30, 'B', -1 ) );
  board.push_back( new Gamepiece( 30, 'Q', -1 ) );
  board.push_back( new Gamepiece( 30, '@', -1  ) );
  board.push_back( new Gamepiece( 30, 'B', -1  ) );
  board.push_back( new Gamepiece( 30, 'K', -1  ) );
  board.push_back( new Gamepiece( 30, 'R', -1  ) );
  for( int i=0; i<board.size(); i++ )
    {
      if( i<16 )
	{
	  board[i]->setOwner( player2uid );
	}
      else if( i>=48 )
	{
	  board[i]->setOwner( player1uid );
	}
      else board[i]->setOwner(0);
    }
}



bool Chess::isOccupied( int position )
{
#ifdef DEBUG
  cerr << "Checking if space #" << position << " is occupied... ";
#endif
  if( position < 0 ) return true;
  if( board[position]->getCharacter()->at(0) == ' ' )
    {
#ifdef DEBUG
      cerr << "nope." << endl;
#endif
      return false;
    }
  else
    {
#ifdef DEBUG
      cerr << "yup." << endl;
#endif
      return true;
    }
}

void Chess::displayBoard(int sd)
{
  // Board Title?
  //send( sd, "\033[1;37mMy Board\033[0;0m\r\n", sizeof("\033[1;37mMy Board\033[0;0m\r\n"), 0 );
  int row=1;
  send( sd, "\r\n A  B  C  D  E  F  G  H\r\n", strlen( "\r\n A  B  C  D  E  F  G  H\r\n"), 0 );
  for( int i=0; i<board.size(); i++ )
    {
      send( sd, board[i]->getBackground()->c_str(), strlen( board[i]->getBackground()->c_str() ), 0 );
      send( sd, board[i]->getForeground()->c_str(), strlen( board[i]->getForeground()->c_str() ), 0 );
      send( sd, " ", strlen( " "), 0 );
      send( sd, board[i]->getCharacter()->c_str(), strlen( board[i]->getCharacter()->c_str() ), 0 );
      send( sd, " ", strlen( " "), 0 );
      send( sd, board[i]->getReset()->c_str(), strlen( board[i]->getReset()->c_str() ), 0 );
      char row_text[] = "  \r\n";
      if( (i+1)%8==0 )
	{
	  row_text[1]=(char)row+48;
	  row++;
	  send( sd, row_text, strlen(row_text), 0 );
	}     
    }  
  send( sd, "\r\n> ", strlen("\r\n> "), 0 );
}

void Chess::displayBoard() // on the Server-side
{
  cerr << " \033[1;43mMy Board\033[0;0m\r\n" << endl;  
  for( int i=0; i<board.size(); i++ )
    {
      cerr << *board[i];
      if( (i+1)%8==0 ) cerr << endl;
    }
  cerr << endl;
}

void Chess::validMoves(int position)
{
  if( !isOnBoard(position) ) return; // check to make sure the starting position is actually on the chess board
  
  //valid_moves.erase( valid_moves.begin(), valid_moves.begin()+valid_moves.size() );
  
  int row=getRow(position);
  int col=getColumn(position);

  switch( board[position]->getCharacter()->at(0) )
    {
    case 'R':
      {
#ifdef DEBUG
	cerr << "checking R: row: " << row << "\tcol: " << col << endl;
#endif
	// Check North
	for( int i=(position-8); /*row>=0  && */ isOnBoard(i) ; i-=8 )
	  {
	    if( !isOccupied(i) && i>0 && i<8)
	      {
		valid_moves.push_back(i);
	      }
	    else
	      {
		// if it's not our own
		if( !isSameOwner(i,position) ) valid_moves.push_back(i);		
		break;
	      }
	  }
	
	// Check South
	for( int i=(position+8); isOnBoard(i) /* row<=7 && i>=0 && i<64*/; i+=8 )
	  {
	    if( !isOccupied(i)  /* && i>0 && i<8 */)
	      {
		valid_moves.push_back(i);
	      }
	    else
	      {
		// if it's not our own
		if( !isSameOwner(i,position) ) valid_moves.push_back(i);		
		break;
	      }
	  }

	// Check East
	for( int i=(position+1); isOnBoard(i) /* col<=7 && i>=0 && i<64 */; i++ )
	  {
	    if( !isOccupied(i)  && i>=0 && i<8)
	      {
		valid_moves.push_back(i);
	      }
	    else
	      {
		// if it's not our own
	        if( !isSameOwner(i,position) ) valid_moves.push_back(i);		
		break;
	      }
	  }

	// Check West
	for( int i=(position-1); isOnBoard(i) /*col>=0 && i>=0 && i<64*/; i-- )
	  {
	    if( !isOccupied(i)  && i>=0 && i<8 )
	      {
		valid_moves.push_back(i);
	      }
	    else
	      {
		// if it's not our own
	        if( !isSameOwner(i,position) ) valid_moves.push_back(i);		
		break;
	      }
	  }
	
	
      }
      break;
    case 'B':
      {
#ifdef DEBUG

	cerr << "checking B: row: " << row << "\tcol: " << col << endl;
#endif	
	// check diagonal NE
	for( int i=(position-7); ((i>6)&&(i%8!=0)&&(i>=0)); i-=7 )
	  {
	    if( !isOccupied(i) && (i >=0) && (i<64))
	      {
		valid_moves.push_back(i);
	      }
	    else
	      {
		// if it's not our own
		if(  !isSameOwner(i,position) && (i >=0) && (i<64)) valid_moves.push_back(i);		
		break;
	      }
	  }
	
	// check diagonal NW
	for( int i=(position-9); (i>=0)&&(i+1%8)!=0; i-=9 )
	  {
	    if( !isOccupied(i)  && (i >=0) && (i<64))
	      {
		valid_moves.push_back(i);
	      }
	    else
	      {
		if( !isSameOwner(i,position) && (i >=0) && (i<64)) valid_moves.push_back(i);
		break;
	      }
	  }
	// check diagonal SE
	for( int i=(position+9); ((i<64)&&(i%8!=0)&&(i>=0)); i+=9 )
	  {
	    if( !isOccupied(i)  && (i >=0) && (i<64))
	      {
		valid_moves.push_back(i);
	      }
	    else
	      {
		if(  !isSameOwner(i,position)  && (i >=0) && (i<64)) valid_moves.push_back(i);
		break;
	      }
	  }
	// check diagonal NW
	for( int i=(position+7); (i>=0)&&(i+1%8)!=0&&(i<64); i+=7 )
	  {
	    if( !isOccupied(i)  && (i>=0) && (i<64))
	      {
		valid_moves.push_back(i);
	      }
	    else
	      {
		if(  !isSameOwner(i,position)  && (i >=0) && (i<64)) valid_moves.push_back(i);
		break;
	      }
	    
	  }
      }
      break;
    case 'K':
      {
#ifdef DEBUG

	cerr << "checking K: row: " << row << "\tcol: " << col << endl;
#endif
	if( col>0 && row>1 )
	  {
	    if( board[position-17]->getOwner() !=  board[position]->getOwner() )
	      {
		valid_moves.push_back(position-17);
	      }
	  }
	if( col<7 && row>1 )
	  {
	    if( board[position-15]->getOwner() !=  board[position]->getOwner() )
	      {
		valid_moves.push_back(position-15);
	      }
	  }
	if( row>0 && col > 1 )
	  {
	    if( board[position-10]->getOwner() !=  board[position]->getOwner() )
	      {
		valid_moves.push_back(position-10);
	      }
	  }
	if( row>0 && col < 6 )
	  {
	    if( board[position-6]->getOwner() !=  board[position]->getOwner() )
	      {
		valid_moves.push_back(position-6);
	      }
	  }
	if( row<7 && col>1)
	  {
	    if( board[position+6]->getOwner() !=  board[position]->getOwner() )
	      {
		valid_moves.push_back(position+6);
	      }
	  }
	if( row<7 && col<6 )
	  {
	    if( board[position+10]->getOwner() !=  board[position]->getOwner() )
	      {
		valid_moves.push_back(position+10);
	      }
	  }
	if( row<6 && col>0 )
	  {
	    if( board[position+15]->getOwner() !=  board[position]->getOwner() )
	      {
		valid_moves.push_back(position+15);
	      }
	  }
	if( row<6 && col<7 )
	  {
	    if( board[position+17]->getOwner() !=  board[position]->getOwner() )
	      {
		valid_moves.push_back(position+17);
	      }
	  }
      }
      break;
    case 'P':
      {
#ifdef DEBUG

	cerr << "checking P: row: " << row << "\tcol: " << col << endl;
#endif		
	// direction needs to be based on ownership - not position
	int direction=board[position]->getDirection();
	
	if( !isOccupied( position+direction*8 ) && (position+direction*8 >0) && (position+direction*8<64))
	  {
	    // cerr << "pawn - one space" << endl;
	    valid_moves.push_back(position+direction*8);
	  }
	if(  isOccupied( position+direction*8-1 ) && (board[position+direction*8-1]->getOwner() != board[position]->getOwner()) && (position+direction*8-1 >0) && (position+direction*8-1<64))
	  {
	    // cerr << "col diff: " << abs(getColumn(position+direction*8-1)-col) << endl;
	    if( abs(getColumn(position+direction*8-1)-col) == 1 )valid_moves.push_back( position+direction*8-1 );
	  }
	if( isOccupied( position+direction*8+1 ) && (board[position+direction*8+1]->getOwner() != board[position]->getOwner()) && (position+direction*8+1 >0) && (position+direction*8+1<64))
	  {
	    // cerr << "col diff: " << abs(getColumn(position+direction*8-1)-col) << endl;
	    if( abs(getColumn(position+direction*8+1)-col) == 1 )valid_moves.push_back( position+direction*8+1 );
	  }
	if(board[position]->getMoveNumber()==0)
	  {
	    // pawn's 1st move can be two spaces
	    //cerr << "pawn - two spaces: " << position+direction*16 << endl;
	    if( !isOccupied( position+direction*8 ) && !isOccupied( position+direction*16)  && (position+direction*16 >0) && (position+direction*16<64)  && (position+direction*8 >0) && (position+direction*8<64))
	      {
		valid_moves.push_back(position+direction*16);
	      }
	  }
	
      }
      break;
    case 'Q':
      {
#ifdef DEBUG
	cerr << "checking Q: row: " << row << "\tcol: " << col << endl;
#endif
	// Check North
#ifdef DEBUG
	cerr << "N" << endl;
#endif
	for( int i=(position-8); isOnBoard(i); i-=8 )
	  {
#ifdef DEBUG
	    cerr << "checking position: " << i << endl;
#endif
	    if( !isOccupied(i))
	      {
		
		valid_moves.push_back(i);
	      }
	    else
	      {
		// if it's not our own
		if( board[i]->getOwner() != board[position]->getOwner()  && (i>=0) && (i<64)) valid_moves.push_back(i);		
		break;
	      }
	  }
	
	// Check South
#ifdef DEBUG
	cerr << "S" << endl;
#endif
	for( int i=(position+8);  isOnBoard(i); i+=8 )
	  {
	    if( isOnBoard(i) && !isOccupied(i))
	      {
		valid_moves.push_back(i);
	      }
	    else
	      {
		// if it's not our own
		if( board[i]->getOwner() != board[position]->getOwner()  && (i>=0) && (i<64)) valid_moves.push_back(i);		
		break;
	      }
	  }
#ifdef DEBUG
	cerr << "E" << endl;
#endif
	for( int i=(position+1);  isOnBoard(i); i++ )
	  {
	    if( isOnBoard(position+1) && !isOccupied(i))
	      {
		valid_moves.push_back(i);
	      }
	    else
	      {
		// if it's not our own
		if( board[i]->getOwner() != board[position]->getOwner()  && (i>=0) && (i<64)) valid_moves.push_back(i);		
		break;
	      }
	  }
#ifdef DEBUG
	cerr << "W" << endl;
#endif
	for( int i=(position-1);  isOnBoard(i); i-- )
	  {
	    if( !isOccupied(i) && (i>=0) && (i<64) )
	      {
		valid_moves.push_back(i);
	      }
	    else
	      {
		// if it's not our own
		if( board[i]->getOwner() != board[position]->getOwner() && (i>=0) && (i<64) ) valid_moves.push_back(i);		
		break;
	      }
	  }
      }
      
      {
#ifdef DEBUG
	// check diagonal NE
	cerr << "NE" << endl;
#endif
	for( int i=(position-7); ((i>6)&&(i%8!=0)&&(i>=0)); i-=7 )
	  {
	    if( !isOccupied(i)  && (i>=0) && (i<64))
	      {
		valid_moves.push_back(i);
	      }
	    else
	      {
		// if it's not our own
		if( board[i]->getOwner() != board[position]->getOwner() && (i>=0) && (i<64) ) valid_moves.push_back(i);		
		break;
	      }
	  }
#ifdef DEBUG
	// check diagonal NW
	cerr << "NW" << endl;
#endif
	for( int i=(position-9); (i>=0)&&(i+1%8)!=0; i-=9 )
	  {
	    if( !isOccupied(i)  && (i>=0) && (i<64))
	      {
		valid_moves.push_back(i);
	      }
	    else
	      {
		if( board[i]->getOwner() !=  board[position]->getOwner()  && (i>=0) && (i<64)) valid_moves.push_back(i);
		break;
	      }
	  }
#ifdef DEBUG
	// check diagonal SE
	cerr << "SE" << endl;
#endif
	for( int i=(position+9); ((i<64)&&(i%8!=0)&&(i>=0)); i+=9 )
	  {
	    if( !isOccupied(i)  && (i>=0) && (i<64))
	      {
		valid_moves.push_back(i);
	      }
	    else
	      {
		if( board[i]->getOwner() !=  board[position]->getOwner()  && (i>=0) && (i<64)) valid_moves.push_back(i);
		break;
	      }
	  }
#ifdef DEBUG
	// check diagonal SW
       	cerr << "SW" << endl;
#endif
	for( int i=(position+7); (i>=0)&&(i+1%8)!=0&&(i<64); i+=7 )
	  {
	    if( !isOccupied(i) && (i>=0) && (i<64) )
	      {
		valid_moves.push_back(i);
	      }
	    else
	      {
		if( board[i]->getOwner() !=  board[position]->getOwner()  && (i>=0) && (i<64)) valid_moves.push_back(i);
		break;
	      }
	  }
      }
      break;
    case '@':
      {
	int z=0;
#ifdef DEBUG
	cerr << "checking @: row: " << row << "\tcol: " << col << endl;	
	// South
	cerr << ++z << endl;
#endif
      	if( isOnBoard(position+8) &&  board[position+8]->getOwner() != board[position]->getOwner() && row<7)
	  {
	    valid_moves.push_back(position+8);
	  }
#ifdef DEBUG
	
	// North
	cerr << ++z << endl;
#endif
	if( isOnBoard(position-8) && board[position-8]->getOwner() != board[position]->getOwner() && row>0)
	  {
	    valid_moves.push_back(position-8);
	  }
#ifdef DEBUG
	
	// North East
	cerr << ++z << endl;
#endif
	if( isOnBoard(position-7) && board[position-7]->getOwner() != board[position]->getOwner() && row>0 && col<7)
	  {
	    valid_moves.push_back(position-7);
	  }
#ifdef DEBUG

	// South East
	cerr << ++z << endl;
#endif
     	if( isOnBoard(position+9) && board[position+9]->getOwner() != board[position]->getOwner() && row<7 && col<7)
	  {
	    valid_moves.push_back(position+9);
	  }
#ifdef DEBUG

	// North West
	cerr << ++z << endl;
#endif
	if( isOnBoard(position-9) && board[position-9]->getOwner() != board[position]->getOwner() && row>0 && col>0)
	  {
	    valid_moves.push_back(position-9);
	  }
#ifdef DEBUG
	
	// South West
	cerr << ++z << endl;
#endif
	if( isOnBoard(position+7) && board[position+7]->getOwner() != board[position]->getOwner() && row<7 && col>0)
	  {
	    valid_moves.push_back(position+7);
	  }
      }
      break;
    default:
      break;
    }
  
  
#ifdef DEBUG2
  // this will stop the game from being playable
  // because the code relies of the character that
  // is stored in the vector to determine what its
  // valid moves are.  if this is changed to a '.'
  // then the valid moves become whatever the valid
  // moves function knows about pieces called '.'.
  for( int i=0; i<valid_moves.size(); i++ )
    {
      board[valid_moves[i]]->setCharacter( '.' );
      cerr << *board[position] << " at " << position << " could move to: " << valid_moves[i] << endl;
    }
#endif
  
}

Chess::Chess(int a, int b)
{
  whosturn=b;
  player1uid=a;
  player2uid=b;

  initPieces();
  initBackgrounds();
#ifdef DEBUG
  cerr << "Created a chess game between uid: " << a << " and " << b << endl;
#endif  
}

class Client
{
public:
  Client(int s);
  ~Client();
  bool sendText( string * s );
  void setIPAddress( string * s ){ ip_address=s; };
  void setName( string * s );
  void setPort( int p ){ port=p; };
  int getSocketDescriptor();
  string * getIPAddress(){ return ip_address; };
  string * getName(){ return username; };
  int getPort(){ return port; };
  bool isPlaying(){ return currently_playing; };
  void isPlaying( bool b ){ currently_playing = b; };
  Chess * getChess(){ return chess; };
  void setChess( Chess * c ){ chess=c; };
  void setUserID( int u ){uid=u;};
  int getUserID(){return uid;};
private:
  int socket_descriptor;
  int port;
  string * ip_address;
  string * username;
  bool currently_playing;
  Chess * chess;
  int uid;
};

int Client::getSocketDescriptor()
{
  return socket_descriptor;
}

Client::Client( int s )
{
  socket_descriptor=s;
  username = new string( "no username" );
  currently_playing = false;
}

void Client::setName( string * s )
{
  if( username!=0 ) delete username;
  username=s;
}

Client::~Client()
{
  if( ip_address!=0 ) delete ip_address;
  if( username!=0 ) delete username;
  if( chess !=0 ) delete chess;
}

bool Client::sendText( string * s )
{
  bool retVal=true;
  if( send(socket_descriptor, s->c_str(), s->length(), 0) != s->length() )
    {
      cout << "[send error]" << endl;
      retVal=false;
    }
  delete s;
  return retVal;
}

// compare 2 buffers
bool compare( char* a, const char* b )
{
  bool retVal=true;
  int l=strlen( b );
  for( int i=0; i<l; i++ )
    {
      if( a[i]!=b[i] )
	{
	  retVal=false;
	  break;
	}
    }
  return retVal;
}

// *============================
bool Chess::move( string * s, int uid )
{
  if( whosturn!=uid )
    {
      int tmpsd;
      tmpsd=player2->getSocketDescriptor();
      if( uid==player1uid )
	{
	  tmpsd=player1->getSocketDescriptor();
	}
      send( tmpsd, "sorry, it's not your turn.\r\n", strlen("sorry, it's not your turn.\r\n"), 0 );
      return(false);
 
    }

  // this converts the A2B4 style string to something useable by the code
  int from_col = s->at(0)-65;
  int from_row = s->at(1)-49;
  int to_col=s->at(2)-65;
  int to_row=s->at(3)-49;

  int from_i = getIndex( from_row, from_col );
  int to_i = getIndex( to_row, to_col );
  
  if( !isOnBoard( from_i ) || !isOnBoard( to_i ) ) //from_col <0 || from_row<0 || to_col <0 || to_row < 0)
    {
#ifdef DEBUG
      cerr << "one of the locations is NOT on the board: " << from_i << "->" << to_i << endl;
#endif
      int tmpsd;
      tmpsd=player2->getSocketDescriptor();
      if( uid==player1uid )
	{
	  tmpsd=player1->getSocketDescriptor();
	}
      send( tmpsd, "[ERR: INVALID MOVE]\r\n", strlen("[ERR: INVALID MOVE]\r\n"), 0 );
      
      return false;
    }
#ifdef DEBUG
  cerr << *s << " means " << from_row << ":" << from_col << " to " << to_row << ":" << to_col << endl;
#endif
  cerr << "move " << *s << endl;
  
  valid_moves.erase( valid_moves.begin(), valid_moves.begin()+valid_moves.size() );
  validMoves(from_i);
  //
  int x;
  if( whosturn==player1uid )
    {
      x=player2uid;
    }
  else
    {
      x=player1uid;
    }

  if( isValidMove(to_i) )
    {
#ifdef DEBUG
      cerr << "Move is a valid move, but the player is in check at the moment." << endl;
#endif
      Gamepiece * tmp_to = new Gamepiece( *board[to_i] );
      Gamepiece * tmp = new Gamepiece( *board[from_i] );
      delete board[from_i];
      board[from_i] = new Gamepiece( 30, ' ', 0 );
      delete board[to_i];
      board[to_i]=tmp;
      
      initBackgrounds();
      
      if( isCheck( whosturn ))
	{

#ifdef DEBUG
	  cerr << "The player is IN check after that move, so it's not a valid move." << endl;
#endif
	  int tmpsd;
	  tmpsd=player2->getSocketDescriptor();
	  if( uid==player1uid )
	    {
	      tmpsd=player1->getSocketDescriptor();
	    }
	  delete board[from_i];
	  board[from_i] = tmp;
	  delete tmp_to;
	  initBackgrounds();
	  
	  send( tmpsd, "[ERR: INVALID MOVE - YOU'D BE IN CHECK]\r\n", strlen("[ERR: INVALID MOVE - YOU'D BE IN CHECK]\r\n"), 0 );
      
	}
      else
	{
#ifdef DEBUG
	  cerr << "The player is NOT in check after that move, so it IS a valid move." << endl;
#endif
	  delete tmp_to;
	  board[to_i]=tmp;
	  initBackgrounds();


	  if( isPromotion() > 0 )
	    {
	      cerr << "there is a promotion ready!" << endl;

	    }
	  // next person's turn
	  if( whosturn==player1uid )
	    {
	      whosturn=player2uid;
	    }
	  else
	    {
	      whosturn=player1uid;
	    }
	  
	  whosturn=player1uid;
	  int tmpsd = player1->getSocketDescriptor();
	  if( uid==player1uid )
	    {
	      whosturn=player2uid;
	      tmpsd=player2->getSocketDescriptor();
	    }
	  
	  
	  displayBoard( player1->getSocketDescriptor() );
	  displayBoard( player2->getSocketDescriptor() );
	  if( isCheck( whosturn ) )
	    send( tmpsd, "\r\nYOU ARE IN CHECK\r\n> ", strlen( "\r\nYOU ARE IN CHECK\r\n> " ), 0 );
	  send( tmpsd, "\r\nYOUR MOVE\r\n> ", strlen( "\r\nYOUR MOVE\r\n> " ), 0 );
	}
    }
  else
    {
      int tmpsd;
      tmpsd=player2->getSocketDescriptor();
      if( uid==player1uid )
	{
	  tmpsd=player1->getSocketDescriptor();
	}
      send( tmpsd, "[ERR: INVALID MOVE]\r\n", strlen("[ERR: INVALID MOVE]\r\n"), 0 );
    }
  //
  // =========
  
  return true;
}
//*****************************

int main(int argc , char *argv[])   
{
  Chess * c;
  
  int uid=1000;
  vector <Client*> vector_of_clients;
  
  int opt = 1;   
  int master_socket , addrlen , new_socket, activity, i , valread , sd;   
  int max_sd;   
  struct sockaddr_in address;   
  
  char buffer[1025];  //data buffer of 1K  
  
  //set of socket descriptors  
  fd_set readfds;   
  
  // create a master socket  
  if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)   
    {   
      cerr << "socket failed" << endl;
      exit(EXIT_FAILURE);
    }   
  
  // set master socket to allow multiple connections ,  
  // this is just a good habit, it will work without this  
  if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )   
    {
      cerr << "setsockopt" << endl;
      exit(EXIT_FAILURE);   
    }   
  
  //type of socket created  
  address.sin_family = AF_INET;   
  address.sin_addr.s_addr = INADDR_ANY;   
  address.sin_port = htons( PORT );   
 
  //bind the socket to localhost port 5752 (port number is from the #define PORT 5752) 
  if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)   
    {
      cerr << "bind failed" << endl;
      exit(EXIT_FAILURE);   
    }
  
  cerr << "Listening for connections on port " << PORT << endl;
  
  //try to specify maximum of 3 pending connections for the master socket  
  if (listen(master_socket, 3) < 0)   
    {   
      cerr << "listen error" << endl;
      exit(EXIT_FAILURE);   
    }   
  
  //accept the incoming connection  
  addrlen = sizeof(address);   
  puts("Waiting for connections ...");   
  bool q=0;
  while(q==0)   
    {   
      // clear the socket set
      FD_ZERO(&readfds);   
      
      // add master socket to set  
      FD_SET(master_socket, &readfds);   
      max_sd = master_socket;   
      
      //add child sockets to set  
      for ( i = 0 ; i < vector_of_clients.size() ; i++)   
        {   
	  //socket descriptor  
          sd = vector_of_clients[i]->getSocketDescriptor();
	  
	  //if valid socket descriptor then add to read list  
	  if(sd > 0) FD_SET( sd , &readfds);   
	  
	  //highest file descriptor number, need it for the select function  
	  if(sd > max_sd) max_sd = sd;   
        }   
      
      // wait for an activity on one of the sockets , timeout is NULL ,  
      // so wait indefinitely  
      activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);   
      
      if ((activity < 0) && (errno!=EINTR))   
        {
	  cerr << "select error" << endl;
        }   
      
      // If something happened on the master socket ,  
      // then its an incoming connection  
      if (FD_ISSET(master_socket, &readfds))   
        {   
	  if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)   
            {   
	      cerr << "[accept error]" << endl;
	      exit(EXIT_FAILURE);   
            }   
	  
	  // inform server-side console of socket number - used in send and receive commands
	  cerr << "New Connection (fd: " << new_socket << ") (ip: " << inet_ntoa(address.sin_addr) << ") (port: " << ntohs(address.sin_port) << ")" << endl;
	  
	  // Logon Sequence
	  //send new connection greeting message
	  send(new_socket, "\033[1;37mp r o t o v i s i o n   g a m e   s e r v e r\033[1;32m\r\n> ", strlen( "\033[1;37mp r o t o v i s i o n   g a m e   s e r v e r\033[1;32m\r\n> " ),0 );
	  
	  //add new client to the vector of clients
	  vector_of_clients.push_back( new Client( new_socket ));
	  vector_of_clients[vector_of_clients.size()-1]->setPort( ntohs(address.sin_port) );
	  vector_of_clients[vector_of_clients.size()-1]->setIPAddress( new string( inet_ntoa(address.sin_addr) ) );
	  vector_of_clients[vector_of_clients.size()-1]->setUserID( uid++ );
	  
	}   
      
      for (i = 0; i < vector_of_clients.size(); i++)   
        {
	  
	  //sd = vector_of_clients[i]->getSocketDescriptor();
	  if (FD_ISSET( vector_of_clients[i]->getSocketDescriptor(), &readfds))   
            {
	      // Check if it was for closing , and also read the  
	      // incoming message  
	      if ((valread = read( vector_of_clients[i]->getSocketDescriptor(), buffer, 1024)) == 0)   
                {   
		  // Somebody disconnected , get his details and print  
		  getpeername(vector_of_clients[i]->getSocketDescriptor(), (struct sockaddr*)&address , (socklen_t*)&addrlen);
		  cerr << "Client " << *vector_of_clients[i]->getName() << " (" << inet_ntoa(address.sin_addr) << ":" << ntohs(address.sin_port) << ") has disconnected." << endl;
		  // Close the socket and remove it from the vector of clients
		  close( vector_of_clients[i]->getSocketDescriptor());
		  vector_of_clients.erase(vector_of_clients.begin()+i);
                }
	      buffer[valread]=0;
	      if( compare( buffer, "show" ) && vector_of_clients[i]->isPlaying() )
		{
		  vector_of_clients[i]->getChess()->displayBoard( vector_of_clients[i]->getSocketDescriptor() );
		}
	      else if(compare( buffer, "username " ) && strlen(buffer)>11)
		{
		  int l=strlen( buffer )-2;
		  int s=9;
		  int index=0;
		  char tmp_buffer[1025];
		  // clear out the temporary buffer
		  for( int j=0; j<1025; j++ ) tmp_buffer[j]=0;
		  for( int j=s; j<l; j++ ) tmp_buffer[index++]=buffer[j];
		  vector_of_clients[i]->setName( new string(tmp_buffer) );
		}
	      else if(compare( buffer, "list users")  && strlen(buffer)==12)
		{
		  for( int j=0; j<vector_of_clients.size(); j++ )
		    {
		      vector_of_clients[i]->sendText( new string( std::to_string( vector_of_clients[j]->getUserID() ) ) );
		      vector_of_clients[i]->sendText( new string(" ") );
		      
		      string * tmp_name = new string( *vector_of_clients[j]->getName() );
		      vector_of_clients[i]->sendText( tmp_name );
		      if( j==i ) vector_of_clients[i]->sendText( new string("*") );
		      vector_of_clients[i]->sendText( new string("\r\n") );
		    }
		}
	      else if(compare( buffer, "list games")  && strlen(buffer)==12)
		{
		  //vector_of_clients[i]->sendText( new string( "number guess\r\n"));
		  //vector_of_clients[i]->sendText( new string( "tic-tac-toe\r\n"));
		  //vector_of_clients[i]->sendText( new string( "checkers\r\n"));
		  vector_of_clients[i]->sendText( new string( "chess\r\n"));
		}
	      else if(compare( buffer, "test" ) && strlen(buffer)==6)
		{
		  vector_of_clients[i]->sendText( new string( "you passed the test!\r\n" ));		  
		}
	      else if( compare( buffer, "* " ))
		{
		  buffer[0]=' ';
		  // broadcast the message
		  for( int j = 0; j< vector_of_clients.size(); j++ )
		    {
		      vector_of_clients[j]->sendText( new string( "message from user: " ));
		      string * s =  new string( *vector_of_clients[i]->getName() );
		      vector_of_clients[j]->sendText( s  );
		     
		      vector_of_clients[j]->sendText( new string( buffer ));
		      vector_of_clients[j]->sendText( new string( "\r\n> " ));
		    }
		}
	      else if( compare( buffer, "> " ))
		{
		  buffer[0]=' ';
		  if( c!=0 && vector_of_clients[i]->isPlaying() )
		    {
		      

		    
		      string * s1 =  new string( *vector_of_clients[i]->getName() );
		      string * s2 =  new string( *vector_of_clients[i]->getName() );
		      
		      c->getPlayer1()->sendText( new string( "message from user: " ));
		      
		      c->getPlayer1()->sendText( s1 );
		      c->getPlayer1()->sendText( new string( buffer ));
		      c->getPlayer1()->sendText( new string( "\r\n> " ));
		      
		      
		      c->getPlayer2()->sendText( new string( "message from user: " ));;
		      c->getPlayer2()->sendText( s2 );
		      c->getPlayer2()->sendText( new string( buffer ));
		      c->getPlayer2()->sendText( new string( "\r\n> " ));
		    }
		}
	      else if(compare( buffer, "exit")  && strlen(buffer)==6)
		{
		  // notify opponents
		  Chess * c = vector_of_clients[i]->getChess();

		  if( vector_of_clients[i]->isPlaying() && c != NULL )
		    {
		      c->getPlayer1()->sendText( new string( "user has resigned\r\n> " ) );
		      c->getPlayer2()->sendText( new string( "user has resigned\r\n> " ) );
		      c->getPlayer1()->isPlaying(false);
		      c->getPlayer2()->isPlaying(false);
		      
		      delete c;
		    }

		  
		  vector_of_clients[i]->sendText( new string( "> goodbye.\r\n" ));
		  close( vector_of_clients[i]->getSocketDescriptor() );
		  cerr << "Client " << *vector_of_clients[i]->getName() << " (" << *vector_of_clients[i]->getIPAddress() << ":" << vector_of_clients[i]->getPort() << ") has disconnected." << endl;

		  close( vector_of_clients[i]->getSocketDescriptor());

		  vector_of_clients.erase(vector_of_clients.begin()+i);
		}
	      else if(compare( buffer, "play")  && strlen(buffer)==6)
		{
		  if( vector_of_clients[i]->isPlaying() )
		    {
		      vector_of_clients[i]->isPlaying( false );
		    }
		  else
		    {
		      vector_of_clients[i]->isPlaying( true );
		    }
		}
	      else if(compare( buffer, "play chess"))
		{
		  char tmp_buffer[5];// a three digit number means that the largest user id is 999
		  tmp_buffer[0]=buffer[11];
		  tmp_buffer[1]=buffer[12];
		  tmp_buffer[2]=buffer[13];
		  tmp_buffer[3]=buffer[14];
		  tmp_buffer[4]=0;
		  
		  int opponent=atoi( tmp_buffer );
		  
		  //cerr << "Opponent: " << opponent << endl;
		  
		  if( opponent == i )
		    {
		      vector_of_clients[i]->sendText( new string( "[Alas, you cannot play vs. yourself.]\r\n" ));
		    }
		  else
		    {
		      
		      // find the index of the opponent with the uid specified
		      int who_to=0;
		      for( int k=0; k<vector_of_clients.size(); k++ )
			{
			  if( vector_of_clients[k]->getUserID() == opponent )
			    {
			      who_to=k;
			      break;
			    }
			}
		      c = new Chess(vector_of_clients[i]->getUserID(), vector_of_clients[who_to]->getUserID() );
		      vector_of_clients[who_to]->setChess( c );
		      vector_of_clients[i]->setChess( c );


		      c->setPlayer1( vector_of_clients[i] );
		      c->setPlayer2( vector_of_clients[who_to] );
		      
		      
		      //c->displayBoard( i );
		      //c->displayBoard( who_to );
		      
		      
		      // START GAME HERE
		      
		      int l=strlen( buffer )-2;
		      int s=10;
		      int index=0;
		      char tmp_buffer[1025];
		      // clear out the temporary buffer
		      for( int j=0; j<1025; j++ ) tmp_buffer[j]=0;
		      for( int j=s; j<l; j++ ) tmp_buffer[index++]=buffer[j];
		      
		      
		      vector_of_clients[i]->isPlaying( true );
		      vector_of_clients[who_to]->isPlaying( true );
		      
		      c->displayBoard( vector_of_clients[who_to]->getSocketDescriptor() );
		      c->displayBoard( vector_of_clients[i]->getSocketDescriptor() );
		      vector_of_clients[who_to]->sendText( new string( "you have been conscripted into a game by user id: " ) );
		      vector_of_clients[who_to]->sendText( new string( std::to_string( vector_of_clients[i]->getUserID() ) ) );
		      vector_of_clients[who_to]->sendText( new string( " - Your move first\r\n> "  ) );
		      
		    }
		}
	      else if( compare( buffer, "move" ))
		{
		  // chess move
		  char mv_bfr[5];
		  mv_bfr[0]=buffer[5];
		  mv_bfr[1]=buffer[6];
		  mv_bfr[2]=buffer[7];
		  mv_bfr[3]=buffer[8];
		  mv_bfr[4]=0;
		  c->move( new string( mv_bfr), vector_of_clients[i]->getUserID() );
		  //c->displayBoard( c->getPlayer1()->getSocketDescriptor() /*, c->getPlayer1()->getUserID() */);
		  //c->displayBoard( c->getPlayer2()->getSocketDescriptor() /*, c->getPlayer2()->getUserID() */);

		  
		}
	      else if( compare( buffer, "help")  && strlen(buffer)==6)
		{
		  vector_of_clients[i]->sendText( new string( "  system help\r\n"));
		  vector_of_clients[i]->sendText( new string( "---------------\r\n"));
		  vector_of_clients[i]->sendText( new string( "username [name]\r\n"));
		  vector_of_clients[i]->sendText( new string( "list games\r\n"));
		  vector_of_clients[i]->sendText( new string( "play [game] [opponent id]\r\n"));
		  vector_of_clients[i]->sendText( new string( "exit\r\n"));
		  vector_of_clients[i]->sendText( new string( "test\r\n"));
		  vector_of_clients[i]->sendText( new string( "move [valid chess move] ex: move A2B3\r\n"));
		  
		  vector_of_clients[i]->sendText( new string( "list users\r\n"));
		  vector_of_clients[i]->sendText( new string( "* message to broadcast\r\n"));		  
		  
		}
	      else 
                {
		  for( int j = 0; j< vector_of_clients.size(); j++ )
		    {
		      if( vector_of_clients[j]->getSocketDescriptor() == vector_of_clients[i]->getSocketDescriptor() )
			{
			  cerr << "Message from (" << *vector_of_clients[i]->getIPAddress() << ":" << vector_of_clients[i]->getPort() << ") ";
			  if( vector_of_clients[i]->getName() != 0 ) cerr << "(" << *vector_of_clients[i]->getName() << ") ";
			  cerr << ":" << buffer << endl;
			}
		    }
                }
	      // clear out that buffer!
	      if( vector_of_clients[i]->isPlaying() )
		{
		  vector_of_clients[i]->sendText( new string("<- currently in game ->") );
		}
	      for( int k=0; k<1025; k++ ) buffer[k]=0;
	      vector_of_clients[i]->sendText( new string( "\r\n> " ));
            }   
        }   
    }   
  
  return 0;   
}   
