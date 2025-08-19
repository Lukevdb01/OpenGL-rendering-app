#ifdef ENGINE_EXPORT_WIN

int main() {
	auto app = CreateApplication();
	app->Run();
	delete app;
}

#endif // ENGINE_EXPORT_WIN