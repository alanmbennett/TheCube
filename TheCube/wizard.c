#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <semaphore.h>

#include "cube.h"
#include "wizard.h"

void *
wizard_func(void *wizard_descr)
{
  struct cube* cube;
  struct room *newroom;
  struct room *oldroom;
  struct wizard* self;
  struct wizard* other;

  self = (struct wizard*)wizard_descr;
  assert(self);
  cube = self->cube;
  assert(cube);
  
  /* Sets starting room */
  oldroom = cube->rooms[self->x][self->y];
  assert(oldroom);

  sem_wait(&cube->start_sem); // wait for game to start
  
  /* Chooses the new room */
  newroom = choose_room(self);

  /* Infinite loop */
  while (1)
    {
      /* Loops until he's able to get a hold on both the old and new rooms */
      while (1)
      {
          sem_wait(&cube->move_mutex);

          if(cube->game_status == 1)
          {
              kill_wizards(self);
              sem_wait(&cube->demise_sem);
          }
          
          if(self->status == 1)
          {
              printf("Wizard %c%d in room (%d,%d) is frozen in place.\n",
                     self->team, self->id, oldroom->x, oldroom->y);
              
              if(cube->mode == 0)
                  sem_post(&cube->move_mutex);
              else
                  sem_post(&cube->cmd_sem);
              
              continue;
          }
          
          
          printf("Wizard %c%d in room (%d,%d) wants to go to room (%d,%d)\n",
                 self->team, self->id, oldroom->x, oldroom->y, newroom->x, newroom->y);
	  
          if (try_room(self, oldroom, newroom)) // if wizard tries the room, but is full
          {
              printf("Request denied, room (%d, %d) locked!\n", newroom->x, newroom->y);
              /* Waits a random amount of time */
              dostuff();
	      
              /* Chooses a new room */
              newroom = choose_room(self);
	      
              /* Loops back to try to go into the new room wizard wishes to enter */
              if(cube->mode == 0)
                  sem_post(&cube->move_mutex);
              else
                  sem_post(&cube->cmd_sem);
              
              continue;
          }
          else // wizard is able to enter the room
          {
              break; // break out of loop so wizard may enter it
          }
      }
      
      printf("Wizard %c%d in room (%d,%d) moves to room (%d,%d)\n",
	     self->team, self->id, 
	     oldroom->x, oldroom->y, newroom->x, newroom->y);

      /* Fill in */
      /* Self is active and has control over both rooms */
      switch_rooms(self, oldroom, newroom);

      other = find_opponent(self, newroom);
      

      /* If there is not another wizard does nothing */
      if (other == NULL)
      {
          printf("Wizard %c%d in room (%d,%d) finds nobody around \n",
                 self->team, self->id, newroom->x, newroom->y);
          /* Fill in */
      }
        
      else
      {
	  /* Other is from opposite team */
          if (other->team != self->team)
          {


	      /* Checks if the opponent is active */
              if (other->status == 0)
              {
                  printf("Wizard %c%d in room (%d,%d) finds active enemy\n",
                         self->team, self->id, newroom->x, newroom->y);

                  fight_wizard(self, other, newroom);

                  int winner = check_winner(cube);

                  if(winner == 1)
                  {
                      printf("Team A Wins!\n");
                      cube->game_status = 1;
                      print_cube(cube);
                      sem_post(&cube->cmd_sem);
                      continue;
                  }
                  else if(winner == 2)
                  {
                      printf("Team B Wins\n");
                      cube->game_status = 1;
                      print_cube(cube);
                      sem_post(&cube->cmd_sem);
                      continue;
                  }
              }
              else
              {
                  printf("Wizard %c%d in room (%d,%d) finds enemy already frozen\n",
                         self->team, self->id, newroom->x, newroom->y);
              }
          }
          /* Other is from same team */
          else
          {
              /* Checks if the friend is frozen */
              if (other->status == 1)
              {
                  free_wizard(self, other, newroom);
              }
          }

	  /* Fill in */
      }
        
      if(cube->mode == 0)
         sem_post(&cube->move_mutex);
      else
         sem_post(&cube->cmd_sem);

      /* Thinks about what to do next */
      dostuff();

      oldroom = newroom;
      newroom = choose_room(self);
        
    }
  
  return NULL;
}

