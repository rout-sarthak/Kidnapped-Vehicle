/*
 * particle_filter.cpp
 *
 *  Created on: Dec 12, 2016
 *      Author: Tiffany Huang
 *      Submitted by: Sarthak Rout
 */

#include <random>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <math.h> 
#include <iostream>
#include <sstream>
#include <string>
#include <iterator>

#include "particle_filter.h"

using namespace std;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
	// TODO: Set the number of particles. Initialize all particles to first position (based on estimates of 
	//   x, y, theta and their uncertainties from GPS) and all weights to 1. 
	// Add random Gaussian noise to each particle.
	// NOTE: Consult particle_filter.h for more information about this method (and others in this file).
	default_random_engine gen;
	double std_x, std_y, std_theta;
	std_x = std[0];
	std_y = std[1];
	std_theta = std[2];

	normal_distribution<double> dist_x(x, std_x);
	normal_distribution<double> dist_y(y, std_y);
	normal_distribution<double> dist_theta(theta, std_theta);
	struct Particle particle;
	for( int i = 0; i < num_particles; i++)
	{
		particle.id = i;
		particle.x = dist_x(gen);
		particle.y = dist_y(gen);
		particle.theta = dist_theta(gen);
		particle.weight = 1.0;
		particles.push_back(particle);
	}
	is_initialized = true;
}

void ParticleFilter::prediction(double delta_t, double std_pos[], double velocity, double yaw_rate) {
	// TODO: Add measurements to each particle and add random Gaussian noise.
	// NOTE: When adding noise you may find std::normal_distribution and std::default_random_engine useful.
	//  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
	//  http://www.cplusplus.com/reference/random/default_random_engine/
	default_random_engine gen;
	double std_x, std_y, std_theta;
	std_theta = std_pos[2];
	std_x = std_pos[0];
	std_y = std_pos[1];
	double pre_x, pre_y, pre_theta;
	for(int i = 0; i < num_particles; i++)
	{
		if(fabs(yaw_rate) < 0.00001)
		{
			pre_x = particles[i].x + velocity * delta_t * cos(particles[i].theta);
			pre_y = particles[i].y + velocity * delta_t * sin(particles[i].theta);
			pre_theta = particles[i].theta;
		}
		else
		{
			pre_x = particles[i].x + (velocity/yaw_rate)*(sin(particles[i].theta + yaw_rate * delta_t) - sin(particles[i].theta));
			pre_y = particles[i].y + (velocity/yaw_rate)*(cos(particles[i].theta) - cos(particles[i].theta + yaw_rate * delta_t));
			pre_theta = particles[i].theta + yaw_rate * delta_t;
		}
		normal_distribution<double> dist_theta(pre_theta, std_theta);
		normal_distribution<double> dist_x(pre_x, std_x);
		normal_distribution<double> dist_y(pre_y, std_y);
		particles[i].theta =  dist_theta(gen);
		particles[i].x = dist_x(gen);
		particles[i].y = dist_y(gen);
	}

}

void ParticleFilter::dataAssociation(std::vector<LandmarkObs> predicted, std::vector<LandmarkObs>& observations) {
	// TODO: Find the predicted measurement that is closest to each observed measurement and assign the 
	//   observed measurement to this particular landmark.
	// NOTE: this method will NOT be called by the grading code. But you will probably find it useful to 
	//   implement this method and use it as a helper during the updateWeights phase.
	   // grab current observation
	  for (unsigned int i = 0; i < observations.size(); i++) 
	  {
	    
	    // grab current observation
	    LandmarkObs o = observations[i];

	    // init minimum distance to maximum possible
	    double min_dist = numeric_limits<double>::max();

	    // init id of landmark from map placeholder to be associated with the observation
	    int map_id = -1;
	    
	    for (unsigned int j = 0; j < predicted.size(); j++) 
	    {
	      // grab current prediction
	      LandmarkObs p = predicted[j];
	      
	      // get distance between current/predicted landmarks
	      double cur_dist = dist(o.x, o.y, p.x, p.y);

	      // find the predicted landmark nearest the current observed landmark
	      if (cur_dist < min_dist) 
	      {
	        map_id = p.id;
	        min_dist = cur_dist;
	      }
	    }

	    // set the observation's id to the nearest predicted landmark's id
	    observations[i].id = map_id;
	}
}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
		const std::vector<LandmarkObs> &observations, const Map &map_landmarks) {
	// TODO: Update the weights of each particle using a mult-variate Gaussian distribution. You can read
	//   more about this distribution here: https://en.wikipedia.org/wiki/Multivariate_normal_distribution
	// NOTE: The observations are given in the VEHICLE'S coordinate system. Your particles are located
	//   according to the MAP'S coordinate system. You will need to transform between the two systems.
	//   Keep in mind that this transformation requires both rotation AND translation (but no scaling).
	//   The following is a good resource for the theory:
	//   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
	//   and the following is a good resource for the actual equation to implement (look at equation 
	//   3.33
	//   http://planning.cs.uiuc.edu/node99.html

	//for loop each particle
	for(int i = 0; i < num_particles; i ++)
	{
		double part_x = particles[i].x;
		double part_y = particles[i].y;
		double part_theta = particles[i].theta;

		//Step 1 Filter map landmark
		//save the map landmark in the range of particle
		//for loop each map landmark
		std::vector<LandmarkObs>  predictions;
		for(int j = 0; j < map_landmarks.landmark_list.size(); j++)
		{
			int map_id = map_landmarks.landmark_list[j].id_i;
			float map_x = map_landmarks.landmark_list[j].x_f;
			float map_y = map_landmarks.landmark_list[j].y_f;
			
			if(dist(map_x, map_y, part_x, part_y) <= sensor_range)
			{
				predictions.push_back(LandmarkObs{map_id, map_x, map_y});
			}
		}
		//Step 2 Transformations
		//The first task is to transform each observation marker from the vehicle's coordinates to the map's coordinates,
		std::vector<LandmarkObs>  transformations;
		for(int j = 0; j < observations.size(); j++)
		{
			double tran_x = cos(part_theta) * observations[j].x - sin(part_theta) * observations[j].y + part_x;
			double tran_y = sin(part_theta) * observations[j].x + cos(part_theta) * observations[j].y + part_y;
			transformations.push_back(LandmarkObs{observations[j].id, tran_x, tran_y});
		}

		//Step3 Association
		dataAssociation(predictions, transformations);
		//Step4 Update Weights
		//reinit weight
		particles[i].weight = 1.0;
		for(int j = 0; j < transformations.size(); j++)
		{
			double sig_x = std_landmark[0];
			double sig_y = std_landmark[1];
			double x_obs, y_obs, mu_x, mu_y;
			x_obs = transformations[j].x;
			y_obs = transformations[j].y;
			int association_id = transformations[j].id;
			for(int k = 0; k < predictions.size(); k++)
			{
				if(predictions[k].id == association_id)
				{
					mu_x = predictions[k].x;
					mu_y = predictions[k].y;
				}
			}
			double weight = ( 1/(2*M_PI*sig_x*sig_y)) * exp( -( pow(x_obs-mu_x,2)/(2*pow(sig_x, 2)) + (pow(y_obs-mu_y,2)/(2*pow(sig_y, 2))) ) );
			particles[i].weight *= weight;
		}
	}
}

void ParticleFilter::resample() {
	// TODO: Resample particles with replacement with probability proportional to their weight. 
	// NOTE: You may find std::discrete_distribution helpful here.
	//   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
	std::vector<Particle> new_particles;
	std::vector<double> weights;
	for(int i = 0; i < num_particles; i++)
	{
		weights.push_back(particles[i].weight);
	}
	//using resampling wheel
	default_random_engine gen;
	uniform_int_distribution<int> uniintdist(0, num_particles-1);
	auto index = uniintdist(gen);
	//get max weight
	double max_weight = *max_element(weights.begin(), weights.end());

	// uniform random distribution [0.0, max_weight)
	uniform_real_distribution<double> unirealdist(0.0, max_weight);

	double beta = 0.0;

	// spin the resample wheel!
	for (int i = 0; i < num_particles; i++) 
	{
	    beta += unirealdist(gen) * 2.0;
	    while (beta > weights[index]) 
	    {
	      beta -= weights[index];
	      index = (index + 1) % num_particles;
	    }
	    new_particles.push_back(particles[index]);
	}

	particles = new_particles;  
}

Particle ParticleFilter::SetAssociations(Particle& particle, const std::vector<int>& associations, 
                                     const std::vector<double>& sense_x, const std::vector<double>& sense_y)
{
    //particle: the particle to assign each listed association, and association's (x,y) world coordinates mapping to
    // associations: The landmark id that goes along with each listed association
    // sense_x: the associations x mapping already converted to world coordinates
    // sense_y: the associations y mapping already converted to world coordinates

    particle.sense_x = sense_x;
    particle.sense_y = sense_y;
    particle.associations= associations;
}

string ParticleFilter::getAssociations(Particle best)
{
	vector<int> v = best.associations;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<int>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseX(Particle best)
{
	vector<double> v = best.sense_x;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseY(Particle best)
{
	vector<double> v = best.sense_y;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
