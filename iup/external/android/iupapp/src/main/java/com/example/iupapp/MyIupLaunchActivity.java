package com.example.iupapp;

import io.github.gen2brain.iupgo.IupLaunchActivity;


public class MyIupLaunchActivity extends IupLaunchActivity
{
	@Override
	protected String[] getLibraries()
	{
		return new String[] { "iupapp" };
	}

	@Override
	public String getEntryPointLibraryName()
	{
		return "libiupapp.so";
	}
}
