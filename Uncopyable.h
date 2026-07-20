#pragma once

class Uncopyable
{
protected:
	Uncopyable() {}
private:
	Uncopyable(const Uncopyable&) = delete;
	Uncopyable& operator =(const Uncopyable&) = delete;
};
