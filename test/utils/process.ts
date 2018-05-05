import * as child_process from 'child_process';
import chalk from 'chalk';
const kill = require('tree-kill');

interface ExecOptions {
  silent?: boolean;
}

interface ProcessOutput {
  stdout: string;
  stderr: string;
  code: number;
}

let _processes: child_process.ChildProcess[] = [];

function _run(options: ExecOptions, cmd: string, args: string[]): Promise<ProcessOutput> {
  return new Promise((resolve, reject) => {
    let stdout = '';
    let stderr = '';
    const cwd = process.cwd();

    args = args.filter(x => x !== undefined);
    const flags = [
      options.silent || false
    ]
      .filter(x => !!x)
      .join(', ')
      .replace(/^(.+)$/, ' [$1]');

    const spawnOptions: any = { cwd };

    if (process.platform.startsWith('win')) {
      args.unshift('/c', cmd);
      cmd = 'cmd.exe';
      spawnOptions['stdio'] = 'pipe';
    }

    const childProcess = child_process.spawn(cmd, args, spawnOptions);

    _processes.push(childProcess);

    childProcess.stdout.on('data', (data: Buffer) => {
      setTimeout(() => resolve(), 100);

      stdout += data.toString();
      if (options.silent) {
        return;
      }

      data.toString()
        .split(/[\n\r]+/)
        .filter(line => line !== '')
        .forEach(line => console.log('  ' + line));
    });

    childProcess.stderr.on('data', (data: Buffer) => {
      resolve();

      stderr += data.toString();
      if (options.silent) {
        return;
      }

      data.toString()
        .split(/[\n\r]+/)
        .filter(line => line !== '')
        .forEach(line => console.error(chalk.yellow('  ' + line)));
    });

    childProcess.on('close', (code: number) => {
      if (code === 0) {
        resolve({ stdout, stderr, code });
      } else {
        const err = new Error(`Running "${cmd} ${args.join(' ')}" returned error code `);
        reject(err);
      }
    });
  });
}

function _exec(options: ExecOptions, cmd: string, args: string[]): Promise<ProcessOutput> {
  return new Promise((resolve, reject) => {
    let stdout = '';
    let stderr = '';
    const cwd = process.cwd();
    console.log(
      `==========================================================================================`
    );

    args = args.filter(x => x !== undefined);
    const flags = [
      options.silent || false
    ]
      .filter(x => !!x)
      .join(', ')
      .replace(/^(.+)$/, ' [$1]');

    console.log(chalk.blue(`Running \`${cmd} ${args.map(x => `"${x}"`).join(' ')}\`${flags}...`));
    console.log(chalk.blue(`CWD: ${cwd}`));
    const spawnOptions: any = { cwd, env: Object.assign({}, process.env, { FORCE_COLOR: true }) };

    if (process.platform.startsWith('win')) {
      args.unshift('/c', cmd);
      cmd = 'cmd.exe';
      spawnOptions['stdio'] = 'pipe';
    }

    const childProcess = child_process.spawn(cmd, args, spawnOptions);

    _processes.push(childProcess);

    childProcess.stdout.on('data', (data: Buffer) => {
      stdout += data.toString();
      if (options.silent) {
        return;
      }

      data.toString()
        .split(/[\n\r]+/)
        .filter(line => line !== '')
        .forEach(line => console.log('  ' + line));
    });

    childProcess.stderr.on('data', (data: Buffer) => {
      stderr += data.toString();
      if (options.silent) {
        return;
      }

      data.toString()
        .split(/[\n\r]+/)
        .filter(line => line !== '')
        .forEach(line => console.error(chalk.yellow('  ' + line)));
    });

    childProcess.on('close', (code: number) => {
      resolve({ stdout, stderr, code });
    });
  });
}

export function exec(cmd, args) {
  return _exec({ silent: false }, cmd, args);
}

export function execSilent(cmd, args) {
  return _exec({ silent: true }, cmd, args);
}

export function killAll(signal = 'SIGTERM'): Promise<void> {
  return Promise.all(_processes.map(process => killProcess(process.pid, signal)))
    .then(() => { _processes = []; });
}

export function killProcess(pid: number, signal = 'SIGTERM'): Promise<null> {
  return new Promise((resolve, reject) => {
    kill(pid, signal, err => {
      if (err) {
        reject();
      } else {
        resolve();
      }
    });
  });
}

export function exitCode(cmd: string): Promise<any> {
  return new Promise((resolve, reject) => {
    console.log(chalk.blue(`Running \`${cmd}\`...`));
    child_process.exec(cmd).on('exit', code => {
      resolve(code);
    });
  });
}

export function bproxy(verbose = false, args: string[] = ['-c', 'bproxy.json']) {
  return _run({ silent: !verbose }, './out/Release/bproxy', args);
}

export function runNode(verbose = false, args: string[]) {
  return _run({ silent: !verbose }, "node", args);
}
