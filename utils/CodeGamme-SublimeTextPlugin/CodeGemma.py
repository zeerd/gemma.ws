import sublime, sublime_plugin
import threading
import json
import CodeGemma.websocket

class CodeGemmaCommand(sublime_plugin.TextCommand):

    def gemma_progress(self):
        index = 0
        while self.view.get_status('CodeGemmaCommand') != "":
            bar = "-"*index + "=" + "-"*(4-index)
            index = index + 1
            if index == 5:
                index = 0
            self.view.set_status('CodeGemmaCommand', 'CodeGemma<' + bar + '>')
            time.sleep(0.5)

    def get_token(self, msg):
        token = json.loads(msg)
        return token["id"], token["token"]

    def gemma_thread(self):
        settings = sublime.load_settings('CodeGemma.sublime-settings')
        service = settings.get('Service', 'ws://localhost:9999')
        oneline = settings.get('OneLine', 'true')

        self.view.set_status('CodeGemmaCommand', 'CodeGemma<----->')
        gemma_progress = threading.Thread(target=self.gemma_progress,
            daemon=True)
        gemma_progress.start()

        data = self.view.substr(sublime.Region(0, self.view.size()))
        cursor = self.view.sel()[0].begin()

        if oneline == "true":
            line = self.view.line(cursor)
            data_before = data[line.begin():line.end()].strip()
            data_after = ""
        else:
            data_before = data[0:cursor]
            data_after = data[cursor:]

        print('Received: ' + data_before)
        print('Received: ' + data_after)

        window = sublime.active_window()
        output = window.create_output_panel("CodeGemmaResults")
        output.run_command('erase_view')
        window.run_command("show_panel", {"panel": "output.CodeGemmaResults"})
        output.run_command('append', {'characters': "Prefilling"})

        ws = CodeGemma.websocket.WebSocket()
        ws.connect(service)

        filename = self.view.file_name().split('/')[-1]
        data = { 'id' : 0, 'prompt' : "" + filename + " <|fim_prefix|>" + data_before +
                " <|fim_suffix|> " + data_after + " <|fim_middle|>" }
        ws.send(json.dumps(data))
        token_id = ""
        token_txt = ""
        while token_txt != "<|fim_middle|>":
            token_id, token_txt = self.get_token(ws.recv())
            print(token_txt)
            output.run_command('append', {'characters': "."})

        output.run_command('append', {'characters': "\n\n"})

        while True:
            token_id, token_txt = self.get_token(ws.recv())
            print(token_txt)
            if token_txt == "<|file_separator|>":
                break
            output.run_command('append', {'characters': token_txt})
        ws.close()

        output.run_command('append', {'characters': "\nDone!"})
        window.run_command("show_panel", {"panel": "output.CodeGemmaResults"})

        self.view.erase_status('CodeGemmaCommand');

    def run(self, edit):
        gemma_thread = threading.Thread(target=self.gemma_thread,
            daemon=True)
        gemma_thread.start()
