#pragma once

#define NR_snapshot 177

extern
int take_snapshot(const char __user * trunk,
		  const char __user * parent,
		  const char __user * name );
