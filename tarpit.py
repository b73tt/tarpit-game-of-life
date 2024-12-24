#!/usr/bin/env python3

import asyncio

async def telnettarpit(reader, writer): # Telnet tarpit - slowly generates login prompts that will always fail, saves incoming data to queue
    global queue

    ip = writer.get_extra_info('peername')[0]
    try:
        while True:
            writer.write(b'\xff\xfd\x01')
            data = await reader.read(100)
            if data:
                queue.put_nowait((ip, data))

            writer.write(b'login: ')
            data = await reader.read(100)
            if data:
                queue.put_nowait((ip, data))
            await asyncio.sleep(3)

            writer.write(b'password: ')
            data = await reader.read(100)
            if data:
                queue.put_nowait((ip, data))
            
            await asyncio.sleep(5)
            await writer.drain()
    except Exception as e:
        print("telnet tarpit exception:")
        print(e)

async def gentarpit(reader, writer): # Generic tarpit - just listens and gathers new data for the queue, for some protocols this is enough
    global queue

    ip=writer.get_extra_info('peername')[0]
    try:
        data = await reader.read(100)
        if data:
            queue.put_nowait((ip, data))
        await asyncio.sleep(25)
        writer.write(b'')
        await writer.drain()
        writer.close()
    except Exception as e:
        print("generic tarpit exception:")
        print(e)

async def comms(reader, writer): # Handles communications with the Inkplate Game of Life board
    global queue
    global conncount
    
    header = await reader.read(100)
    if header[0:9] != b'GET /CGoL': return # The valid client will always send this get request, kinda security by obscurity to require the "/CGoL" but this isn't sensitive data, we just want to avoid random connections stealing our precious entropy

    ip = None
    data = None
    message = b''
    try: #If there's existing data in the queue, pop the first message, if there's no data in the queue timeout after 2 seconds so the client isn't waiting for long
        (ip, data) = await asyncio.wait_for(queue.get(), timeout=2)

        # Get the IP address as four bytes then the data
        for octet in ip.split("."):
            message += str.encode(chr(int(octet)))
        message += data
    except:
        pass #No biggie if it timed out, this prob just means there isn't any data in the queue

    # Finally, send the data if there is any and close the connection
    writer.write(b'HTTP/1.1 200 OK\r\n\r\n'+message)
    await writer.drain()
    writer.close()

async def main():
    global queue
    global conncount
    conncount = 0
    queue = asyncio.Queue()
    tarpitserver = await asyncio.start_server(telnettarpit, '0.0.0.0', 23) #telnet
    gentarpitserver = await asyncio.start_server(gentarpit, '0.0.0.0', 3389) #rdp
    commsserver = await asyncio.start_server(comms, '0.0.0.0', 2383) #communications channel
    async with tarpitserver:
        await tarpitserver.serve_forever()

asyncio.run(main())

