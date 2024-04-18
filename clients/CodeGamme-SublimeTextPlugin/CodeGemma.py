import sublime, sublime_plugin
import threading
import json
import time
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
        return token["id"], token["choices"]["messages"]["content"]

    def get_percent(self, msg):
        token = json.loads(msg)
        a = token["usage"]["completion_tokens"]
        b = token["usage"]["total_tokens"]
        p = 0
        if b > 0:
            p = a * 100 / b;
        return str(round(p, 2)) + "%"

    def get_fun_range(self):
        row = 0
        last_row = 0
        start = 0
        end = self.view.size()
        for region in  self.view.sel():
            region_row, region_col =  self.view.rowcol(region.begin())
            function_regions = self.view.find_by_selector('meta.function - meta.function.inline')
            if function_regions:
                for r in reversed(function_regions):
                    start = self.view.line(r).begin()
                    row, col = self.view.rowcol(r.begin())
                    if row <= region_row:
                        lines = self.view.substr(r)
                        break
                    else:
                        last_row = row
                    end = self.view.line(r).begin()
        print(start)
        print(end)
        return start, end

    def gemma_thread(self):
        settings = sublime.load_settings('CodeGemma.sublime-settings')
        service = settings.get('Service', 'ws://localhost:9999')
        wholefile = settings.get('WholeFile', 'false')

        self.view.set_status('CodeGemmaCommand', 'CodeGemma<----->')
        gemma_progress = threading.Thread(target=self.gemma_progress,
            daemon=True)
        gemma_progress.start()

        data = self.view.substr(sublime.Region(0, self.view.size()))
        cursor = self.view.sel()[0].begin()

        start, end = self.get_fun_range()
        if (wholefile) and (start != end):
            data_before = data[start:cursor]
            data_after = data[cursor:end]
        else:
            data_before = data[0:cursor]
            data_after = data[cursor:]

        print('fim_prefix: ' + data_before)
        print('fim_suffix: ' + data_after)

        window = sublime.active_window()
        output = window.create_output_panel("CodeGemmaResults")
        output.run_command('erase_view')
        window.run_command("show_panel", {"panel": "output.CodeGemmaResults"})
        output.run_command('append', {'characters': "Prefilling"})

        ws = CodeGemma.websocket.WebSocket()
        ws.connect(service)

        filename = self.view.file_name().split('/')[-1]
        data = { 'id' : 0,
                 'messages' : { 'content' : "" + filename + " <|fim_prefix|>" + data_before +
                " <|fim_suffix|> " + data_after + " <|fim_middle|>" } }
        ws.send(json.dumps(data))

        token_id = ""
        token_txt = ""
        while token_txt != "<|fim_middle|>":
            token_id, token_txt = self.get_token(ws.recv())
            output.run_command('erase_view')
            output.run_command('append', {'characters' : " " + self.get_percent(ws.recv()) })

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
